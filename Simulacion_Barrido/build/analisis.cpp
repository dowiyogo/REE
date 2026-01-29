#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TMath.h"
#include <iostream>
#include <vector>

// Función auxiliar para integrar el pico (suma de cuentas) desde un TTree
double ObtenerCuentas(const char* nombreArchivo, double &error) {
    TFile* f = TFile::Open(nombreArchivo);
    if(!f || f->IsZombie()) {
        std::cerr << "Error abriendo " << nombreArchivo << std::endl;
        return 0;
    }
    
    // 1. Obtener el TTree "Scoring"
    TTree* t = (TTree*)f->Get("Scoring");
    if(!t) {
        std::cerr << "No se encontró el TTree 'Scoring' en " << nombreArchivo << std::endl;
        f->Close();
        return 0;
    }

    // 2. Crear un histograma temporal para proyectar los datos
    // Rango: 0 a 3 MeV (cubre Am-241 y Na-22), 4096 bins para buena resolución
    TH1D* h = new TH1D("hTemp", "Espectro Proyectado", 4096, 0.0, 3.0);
    
    // 3. Proyectar la rama "Energy" en el histograma
    // La sintaxis "Energy>>hTemp" llena el histograma creado arriba
    t->Draw("Energy>>hTemp", "", "goff"); // "goff" = graphics off (no dibuja en pantalla aun)

    // 4. Integrar todo el histograma (o definir ROI más adelante)
    double cuentas = h->IntegralAndError(1, h->GetNbinsX(), error);
    
    // Limpieza
    delete h;
    f->Close();
    
    return cuentas;
}

void analisis() {
    // Datos de entrada (Simulados)
    // Definimos las concentraciones de Tierras Raras (REE)
    const int n = 6;
    double conc[n] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0}; // Porcentajes %
    
    // Archivos (Asegúrate de que los nombres coincidan con tu salida de Geant4)
    std::vector<const char*> archivos = {
        "Am241_0p0_REE.root", 
        "Am241_0p01_REE.root", 
        "Am241_0p02_REE.root", 
        "Am241_0p03_REE.root", 
        "Am241_0p04_REE.root", 
        "Am241_0p05_REE.root"
    };

    std::vector<double> I_Am(n), e_Am(n);

    // --- 1. LECTURA DE DATOS ---
    std::cout << "--- LEYENDO DATOS SIMULADOS (Am-241) ---" << std::endl;
    for(int i=0; i<n; i++) {
        I_Am[i] = ObtenerCuentas(archivos[i], e_Am[i]);
        
        // Simulación de error si es 0 (solo para evitar nan en pruebas vacías)
        if(I_Am[i] == 0) { I_Am[i] = 1000; e_Am[i] = sqrt(1000); } 

        std::cout << "Conc: " << conc[i] << "% -> Cuentas Am-241: " << I_Am[i] 
                  << " +/- " << e_Am[i] << std::endl;
    }

    // --- 3. CÁLCULO DE LA TRANSMISIÓN RELATIVA (T) ---
    // T = I(x) / I(0)
    std::vector<double> T_Am(n), eT_Am(n);
    for(int i=0; i<n; i++) {
        T_Am[i] = I_Am[i] / I_Am[0];
        // Propagación de error para división: f = A/B -> (df/f)^2 = (dA/A)^2 + (dB/B)^2
        double term1 = pow(e_Am[i]/I_Am[i], 2);
        double term2 = pow(e_Am[0]/I_Am[0], 2);
        eT_Am[i] = T_Am[i] * sqrt(term1 + term2);
    }

    // --- 4. GRAFICAR ---
    TCanvas* c1 = new TCanvas("c1", "Curva de Calibracion", 800, 600);
    c1->SetGrid();

    // Gráfico de Transmisión vs Concentración
    TGraphErrors* gr = new TGraphErrors(n, &conc[0], &T_Am[0], 0, &eT_Am[0]);
    gr->SetTitle("Sensibilidad Am-241 a Tierras Raras;Concentracion REE (%);Transmision Relativa (I/I_{0})");
    gr->SetMarkerStyle(21);
    gr->SetMarkerColor(kBlue);
    gr->SetLineColor(kBlue);
    
    gr->Draw("ALP");

    // Leyenda
    TLegend* leg = new TLegend(0.6, 0.7, 0.9, 0.9);
    leg->AddEntry(gr, "Simulacion Geant4", "lp");
    leg->Draw();
    
    c1->SaveAs("curva_calibracion_simple.png");
}