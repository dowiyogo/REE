cmake_minimum_required(VERSION 3.16)
project(Simulacion_TierrasRaras)

# Buscar Geant4
find_package(Geant4 REQUIRED ui_all vis_all)

# Configurar compilación
include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

# Buscar todos los archivos fuente y cabeceras
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

# Crear el ejecutable
add_executable(Simulacion_TierrasRaras main.cc ${sources} ${headers})
target_link_libraries(Simulacion_TierrasRaras ${Geant4_LIBRARIES})

# Copiar macros al directorio de construcción
set(MACROS init_vis.mac run.mac experimento_REE.mac)
foreach(macro ${MACROS})
  configure_file(${PROJECT_SOURCE_DIR}/${macro} ${PROJECT_BINARY_DIR}/${macro} COPYONLY)
endforeach()