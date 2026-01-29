#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Color.hh"
#include "G4GenericMessenger.hh" 

// 1. CONSTRUCTOR
DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(), 
  fREEFraction(0.0), 
  fApatiteWithREE(nullptr),
  fLogicSample(nullptr),
  fLogicDetector(nullptr) // <--- AÑADE ESTO (Inicializar a nulo)
{
    // Crear el mensajero
    fMessenger = new G4GenericMessenger(this, "/MedidorTR/det/", "Control del Detector");
    
    // --- VERSIÓN CORREGIDA Y FINAL ---
    // Usamos DeclareMethod normal.
    // Al NO poner .SetUnit(), GEANT4 asume que es un número puro (double).
    // Esto es seguro para hilos (thread-safe) y no requiere unidades inventadas.
    fMessenger->DeclareMethod("setREE", 
                              &DetectorConstruction::SetREEConcentration, 
                              "Set REE concentration (mass fraction 0.0 - 1.0)");
}

// 2. DESTRUCTOR
DetectorConstruction::~DetectorConstruction()
{
    delete fMessenger; 
}

// 3. CONSTRUCT: Aquí definimos el Mundo y la Muestra
G4VPhysicalVolume* DetectorConstruction::Construct()
{
    // Obtener manejador de materiales
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* worldMat = nist->FindOrBuildMaterial("G4_AIR");

    // Material del Detector (Centellador NaI típico)
    //G4Material* detMat = nist->FindOrBuildMaterial("G4_SODIUM_IODIDE"); 

    // Asegurarnos de que el material de la muestra exista
    DefineMaterials();
    // 2. RECUPERAR EL MATERIAL
    // Como 'matLaBr3Ce' era una variable local de la otra función, aquí no existe.
    // Pero el material "LaBr3(Ce)" ya está en la memoria de Geant4. Lo buscamos:
    G4Material* matDetector = G4Material::GetMaterial("LaBr3(Ce)");

    // Verificación de seguridad (buena práctica)
    if (!matDetector) {
        G4cerr << "ERROR CRÍTICO: No se encontró el material LaBr3(Ce)" << G4endl;
        return nullptr;
    }
    // --- A. VOLUMEN MUNDO ---
    G4double worldSize = 1.0 * m;
    G4Box* solidWorld = new G4Box("World", worldSize/2, worldSize/2, worldSize/2);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, worldMat, "World");
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0, true);

    // --- B. VOLUMEN DE LA MUESTRA (El Apatito) ---
    // Está en el centro (0,0,0) con espesor 5cm (de -2.5 a +2.5 cm en Z)
    G4double sampleX = 20.0 * cm; 
    G4double sampleY = 20.0 * cm; 
    G4double sampleZ = 5.0 * cm;  

    G4Box* solidSample = new G4Box("Sample", sampleX/2, sampleY/2, sampleZ/2);
    fLogicSample = new G4LogicalVolume(solidSample, fApatiteWithREE, "Sample");
    new G4PVPlacement(0, G4ThreeVector(0,0,0), fLogicSample, "Sample", logicWorld, false, 0, true);

    // --- CONSTRUCCIÓN DEL VOLUMEN DEL DETECTOR ---

    // Ejemplo de geometría típica de 2x2 pulgadas o similar
    G4double det_R = 2.54*cm; // Radio
    G4double det_Z = 2.54*cm; // Semialtura (Largo total = ~5cm)

    G4Tubs* solidDetector = new G4Tubs("SolidDetector", 
                                   0.*cm,    // Radio interno
                                   det_R,    // Radio externo
                                   det_Z,    // Semilargo Z
                                   0.*deg, 360.*deg);
    // 3. USARLO EN EL VOLUMEN LÓGICO
    fLogicDetector = new G4LogicalVolume(solidDetector, 
                                    matDetector, // Usamos el puntero recuperado
                                     "LogicDetector");

    // (Luego viene el G4PVPlacement usual...)


    // --- C. EL DETECTOR (NUEVO) ---
    // Lo colocamos detrás de la muestra para medir la Transmisión.
    // Si la muestra termina en Z = 2.5cm, pongamos el detector en Z = 15cm.
    
    //G4Box* solidDet = new G4Box("Detector", 10*cm, 10*cm, 5*cm); // Detector de 10cm de profundidad
    
    //fLogicDetector = new G4LogicalVolume(solidDet, detMat, "Detector");
    
    new G4PVPlacement(0, 
                      G4ThreeVector(0, 0, 15.0*cm), // Posición Z = +15 cm
                      fLogicDetector, 
                      "LogicDetector",//"Detector", // Era el de NaI(Tl)
                      logicWorld, 
                      false, 
                      0, 
                      true);

    // --- D. VISUALIZACIÓN ---
    G4VisAttributes* sampleVis = new G4VisAttributes(G4Color(0.0, 1.0, 1.0, 0.6)); // Cyan
    sampleVis->SetForceSolid(true);
    fLogicSample->SetVisAttributes(sampleVis);
    
    G4VisAttributes* detVis = new G4VisAttributes(G4Color(1.0, 0.0, 0.0, 0.5)); // Rojo semi-transparente
    detVis->SetForceSolid(true);
    fLogicDetector->SetVisAttributes(detVis);

    return physWorld; 
}

// 4. DEFINICIÓN DE MATERIALES (Tu código, con pequeña optimización)
void DetectorConstruction::DefineMaterials() {
    G4NistManager* nist = G4NistManager::Instance();

    // =========================================================
    // PARTE 1: MATERIALES FIJOS (Solo se crean una vez)
    // =========================================================
    
    // Preguntamos: ¿NO existe el LaBr3(Ce)?
    if (!G4Material::GetMaterial("LaBr3(Ce)", false)) {
        
        G4cout << "--> Creando materiales fijos (LaBr3, etc.)..." << G4endl;

        // 1. Elementos
        G4Element* elLa = nist->FindOrBuildElement("La");
        G4Element* elBr = nist->FindOrBuildElement("Br");
        G4Element* elCe = nist->FindOrBuildElement("Ce");
        // ... define el resto de elementos aquí (Ca, P, O, F) si no los usas abajo ...

        // 2. Definir LaBr3 Base
        G4Material* LaBr3 = new G4Material("LaBr3_Base", 5.08*g/cm3, 2);
        LaBr3->AddElement(elLa, 1);
        LaBr3->AddElement(elBr, 3);

        // 3. Definir LaBr3(Ce)
        G4Material* matLaBr3Ce = new G4Material("LaBr3(Ce)", 5.08*g/cm3, 2);
        matLaBr3Ce->AddMaterial(LaBr3, 95.0*perCent);
        matLaBr3Ce->AddElement(elCe, 5.0*perCent);
        
        // Crea aquí la base del Apatito (sin REE) si es fija
        G4Element* elCa = nist->FindOrBuildElement("Ca");
        G4Element* elP  = nist->FindOrBuildElement("P");
        G4Element* elO  = nist->FindOrBuildElement("O");
        G4Element* elF  = nist->FindOrBuildElement("F");
        
        G4Material* apatiteBase = new G4Material("ApatiteBase", 3.19*g/cm3, 4);
        apatiteBase->AddElement(elCa, 5);
        apatiteBase->AddElement(elP, 3);
        apatiteBase->AddElement(elO, 12);
        apatiteBase->AddElement(elF, 1);
    }

    // =========================================================
    // PARTE 2: MATERIAL VARIABLE (Se ejecuta SIEMPRE)
    // =========================================================
    // Esta parte DEBE ejecutarse cada vez que cambias la concentración
    
    // Recuperamos los punteros que necesitamos (seguro porque ya pasamos la Parte 1)
    G4Material* apatiteBase = G4Material::GetMaterial("ApatiteBase");
    G4Material* reeMaterial = nist->FindOrBuildMaterial("G4_Ce"); 

    // Definimos el nombre único para esta concentración
    G4String matName = "Apatite_doped_" + std::to_string(fREEFraction);
    
    // Verificamos si ESTA concentración específica ya existe para no duplicarla
    // (Esto evita el warning "duplicate name" en esa parte)
    G4Material* existingMat = G4Material::GetMaterial(matName, false);
    
    if (existingMat) {
        // Si ya existe (ej: volviste a 0% después de probar 5%), úsalo.
        fApatiteWithREE = existingMat;
    } else {
        // Si es nuevo, calcúlalo y créalo
        G4double density = 3.19*g/cm3 * (1.0 - fREEFraction) + 6.77*g/cm3 * fREEFraction;
        
        fApatiteWithREE = new G4Material(matName, density, 2);
        fApatiteWithREE->AddMaterial(apatiteBase, 1.0 - fREEFraction);
        fApatiteWithREE->AddMaterial(reeMaterial, fREEFraction);
        
        G4cout << "--> Material Nuevo Creado: " << matName << G4endl;
    }

    // Actualizar el volumen lógico si ya está construido
    if(fLogicSample) {
        fLogicSample->SetMaterial(fApatiteWithREE);
        // IMPORTANTE: Avisar al RunManager que la geometría cambió
        //G4RunManager::GetRunManager()->GeometryHasBeenModified();
    }
}

// 5. UPDATE
void DetectorConstruction::SetREEConcentration(G4double fraction) {
    fREEFraction = fraction;
    DefineMaterials(); 
    G4RunManager::GetRunManager()->GeometryHasBeenModified();
    // CAMBIO CRÍTICO: Usar ReinitializeGeometry en lugar de GeometryHasBeenModified
    // Esto fuerza una reconstrucción completa incluyendo las tablas de física
    //G4RunManager::GetRunManager()->ReinitializeGeometry(true, false);
}