cmake_minimum_required(VERSION 3.16)
project(Simulacion_Europio)

# Buscar Geant4 con soporte MT, UI y visualización
find_package(Geant4 REQUIRED ui_all vis_all)

# Configurar compilación
include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

# Buscar todos los archivos fuente y cabeceras
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

# Crear el ejecutable
add_executable(Simulacion_Europio main.cc ${sources} ${headers})
target_link_libraries(Simulacion_Europio ${Geant4_LIBRARIES})

# Copiar macros al directorio de construcción
set(MACROS 
    init_vis.mac 
    run_Eu152.mac
    run_background.mac
)
foreach(macro ${MACROS})
  if(EXISTS ${PROJECT_SOURCE_DIR}/${macro})
    configure_file(${PROJECT_SOURCE_DIR}/${macro} ${PROJECT_BINARY_DIR}/${macro} COPYONLY)
  endif()
endforeach()
# Copiar script de barrido
set(SCRIPTS
    run_scan.sh 
    run_scan_fino.sh
)
foreach(script ${SCRIPTS})
  if(EXISTS ${PROJECT_SOURCE_DIR}/${script})
    configure_file(${PROJECT_SOURCE_DIR}/${script} ${PROJECT_BINARY_DIR}/${script} COPYONLY)
  endif()
endforeach()


# Mensaje de configuración
message(STATUS "===========================================")
message(STATUS "Simulacion Europio-152 configurada")
message(STATUS "Geant4 version: ${Geant4_VERSION}")
message(STATUS "MT enabled: ${Geant4_multithreaded_FOUND}")
message(STATUS "===========================================")
