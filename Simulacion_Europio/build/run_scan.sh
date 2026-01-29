#!/bin/bash
# =============================================================
# run_scan.sh - Barrido de concentración REE con fuente Eu-152
# =============================================================

echo "=== INICIO DEL BARRIDO REE con Eu-152 ==="
echo "Fecha: $(date)"
echo ""

# Verificar que existe el ejecutable
EXEC="./Simulacion_Europio"
if [ ! -f "$EXEC" ]; then
    echo "ERROR: No se encuentra $EXEC"
    echo "¿Ejecutaste cmake y make?"
    exit 1
fi

# Número de eventos por concentración
NEVENTS=100000000

# Barrido de concentraciones REE (fracción másica)
for ree in 0.00 0.01 0.02 0.03 0.04 0.05; do
    echo ">>> Ejecutando Eu-152 con REE = $ree"
    
    # Reemplazar punto por 'p' para el nombre del archivo
    ree_name=$(echo $ree | tr '.' 'p')
    
    # Crear macro temporal
    cat > temp_run.mac << EOF
# Macro generada automáticamente para REE = $ree
/run/initialize

# --- 1. CONFIGURACIÓN DE DECAIMIENTO Y FÍSICA ---
# Asegura que GEANT4 procese isótopos con vidas medias largas (Eu-152 ~13.5 años)
# El valor es un límite de tiempo muy alto para "habilitar" el decaimiento.
/process/had/rdm/thresholdForVeryLongDecayTime 1.0e+60 year

# Configurar fuente Eu-152
/gps/particle ion
/gps/ion 63 152 0 0
/gps/energy 0 keV
/gps/pos/type Point
/gps/pos/centre 0. 0. -10. cm
/gps/ang/type iso

# Configurar concentración REE
/MedidorTR/det/setREE $ree

# Archivo de salida
/analysis/setFileName Eu152_REE_${ree_name}

# Ejecutar
/run/beamOn $NEVENTS
EOF
    
    # Ejecutar simulación
    $EXEC temp_run.mac
    
    # Verificar resultado
    if [ $? -ne 0 ]; then
        echo "ERROR en simulación con REE=$ree"
        rm -f temp_run.mac
        exit 1
    fi
    
    echo ">>> Completado REE = $ree"
    echo ""
done

# Limpieza
rm -f temp_run.mac

echo "=== BARRIDO COMPLETADO ==="
echo "Archivos generados:"
ls -la Eu152_REE_*.root 2>/dev/null || echo "No se encontraron archivos .root"
