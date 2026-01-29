#!/bin/bash
# run_scan.sh - Ejecuta el barrido completo de forma segura

echo "=== INICIO DEL BARRIDO REE ==="

# Am-241 (59.5 keV)
for ree in 0.0 0.01 0.02 0.03 0.04 0.05; do
    echo ">>> Ejecutando Am241 con REE = $ree"
    
    # Reemplazar punto por guión bajo para el nombre del archivo
    ree_name=$(echo $ree | tr '.' 'p')
    
    cat > temp_run.mac << EOF
/run/initialize
/gps/particle gamma
/gps/pos/type Point
/gps/pos/centre 0. 0. -10. cm
/gps/direction 0 0 1
/gps/ene/mono 59.5 keV
/analysis/setFileName Am241_${ree_name}_REE.root
/MedidorTR/det/setREE ${ree}
/run/beamOn 10000000
EOF
    
    ./Simulacion_Barrido temp_run.mac
    
    if [ $? -ne 0 ]; then
        echo "ERROR en Am241 REE=$ree"
        exit 1
    fi
done

# Na-22 (511 keV)
for ree in 0.0 0.01 0.02 0.03 0.04 0.05; do
    echo ">>> Ejecutando Na22 con REE = $ree"
    
    ree_name=$(echo $ree | tr '.' 'p')
    
    cat > temp_run.mac << EOF
/run/initialize
/gps/particle gamma
/gps/pos/type Point
/gps/pos/centre 0. 0. -10. cm
/gps/direction 0 0 1
/gps/ene/mono 511. keV
/analysis/setFileName Na22_${ree_name}_REE.root
/MedidorTR/det/setREE ${ree}
/run/beamOn 10000000
EOF
    
    ./Simulacion_Barrido temp_run.mac
    
    if [ $? -ne 0 ]; then
        echo "ERROR en Na22 REE=$ree"
        exit 1
    fi
done

rm -f temp_run.mac
echo "=== BARRIDO COMPLETADO ==="