#!/bin/bash
# =============================================================
# run_scan_fino.sh - Barrido de concentración REE con fuente Eu-152
# Incluye barrido fino (0-1%) para validación de LOD
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
# NOTA: Para el barrido fino cerca del LOD, buena estadística es crítica
NEVENTS=100000000

# =============================================================
# CONCENTRACIONES A SIMULAR
# =============================================================
# Barrido FINO (0% - 1%): Para validar LOD = 0.55% y LOQ = 1.84%
#   0.00 - Referencia (blanco)
#   0.002 - 0.2% (bajo LOD, no debería ser detectable)
#   0.004 - 0.4% (cercano al LOD)
#   0.006 - 0.6% (ligeramente sobre LOD)
#   0.008 - 0.8% (entre LOD y 1%)
#   0.01  - 1.0% (ya existente, entre LOD y LOQ)
#
# Barrido GRUESO (1% - 5%): Para calibración general
#   0.02, 0.03, 0.04, 0.05
# =============================================================

# Array con todas las concentraciones (fracción másica)
# Ordenadas de menor a mayor para facilitar el análisis
CONCENTRACIONES=(
    0.00    # Referencia
    0.002   # 0.2% - Bajo LOD
    0.004   # 0.4% - Cercano a LOD
    0.006   # 0.6% - Sobre LOD
    0.008   # 0.8% - Entre LOD y 1%
    0.01    # 1.0%
    0.02    # 2.0%
    0.03    # 3.0%
    0.04    # 4.0%
    0.05    # 5.0%
)

echo "Concentraciones a simular: ${CONCENTRACIONES[@]}"
echo "Eventos por simulación: $NEVENTS"
echo "Total de simulaciones: ${#CONCENTRACIONES[@]}"
echo ""

# Contador de progreso
TOTAL=${#CONCENTRACIONES[@]}
ACTUAL=0

for ree in "${CONCENTRACIONES[@]}"; do
    ACTUAL=$((ACTUAL + 1))
    
    # Calcular porcentaje para mostrar
    PORCENTAJE=$(echo "$ree * 100" | bc)
    
    echo ">>> [$ACTUAL/$TOTAL] Ejecutando Eu-152 con REE = $ree ($PORCENTAJE%)"
    
    # Reemplazar punto por 'p' para el nombre del archivo
    # Esto maneja correctamente 0.002 -> 0p002
    ree_name=$(echo $ree | tr '.' 'p')
    
    # Verificar si el archivo ya existe (para poder continuar simulaciones interrumpidas)
    OUTPUT_FILE="Eu152_REE_${ree_name}.root"
    if [ -f "$OUTPUT_FILE" ]; then
        echo "    NOTA: $OUTPUT_FILE ya existe, saltando..."
        echo ""
        continue
    fi
    
    # Crear macro temporal
    cat > temp_run.mac << EOF
# =============================================================
# Macro generada automáticamente para REE = $ree ($PORCENTAJE%)
# Fecha: $(date)
# =============================================================

/run/initialize

# --- 1. CONFIGURACIÓN DE DECAIMIENTO Y FÍSICA ---
# Asegura que GEANT4 procese isótopos con vidas medias largas (Eu-152 ~13.5 años)
/process/had/rdm/thresholdForVeryLongDecayTime 1.0e+60 year

# --- 2. CONFIGURACIÓN DE LA FUENTE Eu-152 ---
/gps/particle ion
/gps/ion 63 152 0 0
/gps/energy 0 keV
/gps/pos/type Point
/gps/pos/centre 0. 0. -10. cm
/gps/ang/type iso

# --- 3. CONFIGURACIÓN DE LA MUESTRA ---
# Concentración REE (fracción másica): $ree = $PORCENTAJE%
/MedidorTR/det/setREE $ree

# --- 4. ARCHIVO DE SALIDA ---
/analysis/setFileName Eu152_REE_${ree_name}

# --- 5. EJECUTAR SIMULACIÓN ---
/run/beamOn $NEVENTS
EOF
    
    # Registrar tiempo de inicio
    START_TIME=$(date +%s)
    
    # Ejecutar simulación
    $EXEC temp_run.mac
    
    # Verificar resultado
    if [ $? -ne 0 ]; then
        echo "ERROR en simulación con REE=$ree"
        rm -f temp_run.mac
        exit 1
    fi
    
    # Calcular tiempo transcurrido
    END_TIME=$(date +%s)
    ELAPSED=$((END_TIME - START_TIME))
    ELAPSED_MIN=$((ELAPSED / 60))
    ELAPSED_SEC=$((ELAPSED % 60))
    
    echo ">>> Completado REE = $ree en ${ELAPSED_MIN}m ${ELAPSED_SEC}s"
    echo ""
done

# Limpieza
rm -f temp_run.mac

echo "=== BARRIDO COMPLETADO ==="
echo ""
echo "Resumen de archivos generados:"
echo "------------------------------"
echo ""
echo "BARRIDO FINO (validación LOD):"
for ree in 0.00 0.002 0.004 0.006 0.008 0.01; do
    ree_name=$(echo $ree | tr '.' 'p')
    FILE="Eu152_REE_${ree_name}.root"
    if [ -f "$FILE" ]; then
        SIZE=$(ls -lh "$FILE" | awk '{print $5}')
        echo "  ✓ $FILE ($SIZE)"
    else
        echo "  ✗ $FILE (no encontrado)"
    fi
done

echo ""
echo "BARRIDO GRUESO (calibración):"
for ree in 0.02 0.03 0.04 0.05; do
    ree_name=$(echo $ree | tr '.' 'p')
    FILE="Eu152_REE_${ree_name}.root"
    if [ -f "$FILE" ]; then
        SIZE=$(ls -lh "$FILE" | awk '{print $5}')
        echo "  ✓ $FILE ($SIZE)"
    else
        echo "  ✗ $FILE (no encontrado)"
    fi
done

echo ""
echo "Fecha de finalización: $(date)"
