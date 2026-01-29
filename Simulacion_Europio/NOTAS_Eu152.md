# Notas Técnicas: Simulación Eu-152 con Geant4

## Física del Eu-152

El Europio-152 es una fuente de calibración estándar porque emite múltiples 
gammas bien caracterizados:

| Energía (keV) | Intensidad (%) | Uso típico            |
|---------------|----------------|------------------------|
| 121.8         | 28.4           | Calibración baja E    |
| 244.7         | 7.5            |                        |
| 344.3         | 26.6           | Línea prominente       |
| 411.1         | 2.2            |                        |
| 444.0         | 3.1            |                        |
| 778.9         | 12.9           |                        |
| 867.4         | 4.2            |                        |
| 964.1         | 14.5           |                        |
| 1085.8        | 10.2           |                        |
| 1112.1        | 13.6           |                        |
| 1408.0        | 21.0           | Calibración alta E     |

Con `G4RadioactiveDecayPhysics`, Geant4 simula el esquema de decaimiento 
completo automáticamente.

## Geometría actual

```
         Fuente Eu-152
              |
              v
    z = -10 cm (punto)
              |
              | (aire)
              v
    ┌─────────────────┐
    │                 │  z = -2.5 a +2.5 cm
    │   MUESTRA       │  Apatita + REE
    │   (20x20x5 cm)  │
    └─────────────────┘
              |
              | (aire)
              v
    ┌─────────────────┐
    │   DETECTOR      │  z = 15 cm (centro)
    │   LaBr3(Ce)     │  2x2 pulgadas
    └─────────────────┘
```

## Posibles mejoras para tu simulación

### 1. Emisión isotrópica vs colimada
- `/gps/ang/type iso` → realista (4π)
- `/gps/direction 0 0 1` → haz colimado (menos realista pero más eficiente)

### 2. Actividad de la fuente
Si quieres relacionar con tiempo real:
```
# Para fuente de 1 μCi = 37 kBq
# En 1 segundo: 37000 decaimientos
/run/beamOn 37000
```

### 3. Resolución del detector
LaBr3(Ce) tiene ~3% FWHM a 662 keV. Para simular esto, necesitarías 
agregar un smearing gaussiano en el post-procesamiento (ROOT):
```cpp
// En ROOT
double sigma = 0.03 * energy / 2.355;  // FWHM → sigma
energy_smeared = gRandom->Gaus(energy, sigma);
```

### 4. Background intrínseco del La-138
Tu `run_background.mac` está bien conceptualmente, pero la posición 
debería ser `0. 0. 15. cm` para coincidir con el detector.

## Comandos útiles para debugging

```bash
# Ver física registrada
/process/list

# Ver materiales definidos
/material/nist/listMaterials

# Ver partículas del decaimiento
/tracking/verbose 1
/run/beamOn 1
```
