# Configurar con tests activados
cmake -B build -DBUILD_TESTS=ON -DENABLE_TIMERS=ON -DENABLE_PROFILER=ON

# Compilar
cmake --build build -j

# # Ejecutar tests
# cd build && ctest --output-on-failure