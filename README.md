# Sistema de Gestión de Ficheros Distribuido (PSDI U3)

Este repositorio contiene la estructura base para la práctica de la asignatura, donde el alumnado debe implementar un gestor de ficheros distribuido con cliente y servidor TCP, y opcionalmente un *broker* de balanceo. El proyecto usa CMake para compilar tanto el cliente como el servidor y aprovecha la librería `libFileManager.a` proporcionada por la universidad.

## Estructura de directorios y binarios generados

La plantilla está organizada para que CMake genere de forma independiente los ejecutables principales:

- **server**: binario del servidor remoto que mantiene un directorio `FileManagerDir` propio.
- **client**: binario del cliente que consume el gestor remoto e interpreta las órdenes definidas en `main_fm.cpp`.
- **fileManager**: binario auxiliar que enlaza directamente contra `libFileManager.a` para probar el gestor local (corresponde al ejecutable `FileManager` indicado en el enunciado).
- **broker** (opcional): solo se generará si añades el objetivo y las fuentes correspondientes al CMake.

Los fuentes se agrupan en los subdirectorios `client/` y `server/`, mientras que los ficheros base (`main_fm.cpp`, `filemanager.h`, `libFileManager.a`, `CMakeLists.txt`) residen en la raíz.

### Requisitos previos
- CMake ≥ 3.5
- Compilador C++17
- `make` y `pthread`

## Cómo compilar con la estructura CMake
1. Posiciónate en la raíz del proyecto (donde está `CMakeLists.txt`).
2. Crea un directorio de compilación y entra en él:
   ```bash
   mkdir build
   cd build
   ```
3. Genera los *build files* con CMake apuntando al directorio raíz:
   ```bash
   cmake ..
   ```
4. Compila los ejecutables:
   ```bash
   make
   ```

Al finalizar, obtendrás `server`, `client` y `fileManager` dentro de `build/`. Si añades el broker al CMake, también aparecerá su binario.

## Cómo ejecutar el sistema

### Lanzar un servidor
1. Desde el directorio `build/`, ejecuta:
   ```bash
   ./server <puerto>
   ```
2. El servidor crea y gestiona su propio directorio `FileManagerDir` en el directorio de trabajo actual. Cada instancia mantiene su copia independiente.

### Lanzar múltiples servidores
- Puedes iniciar varias instancias de `server` usando puertos distintos (p. ej., 5000, 5001). Cada una conservará su propio `FileManagerDir`, por lo que los contenidos divergen salvo que los sincronices manualmente.

### Ejecutar un cliente y conectar con el servidor
1. Desde `build/`, lanza el cliente indicando host y puerto del servidor destino:
   ```bash
   ./client <ip-servidor> <puerto>
   ```
2. El cliente interpreta las órdenes de `main_fm.cpp` y actúa contra el gestor remoto.

### Órdenes disponibles (`main_fm.cpp`)
- `ls`: lista los ficheros del directorio local donde se ejecuta el cliente.
- `lls`: lista los ficheros en el `FileManagerDir` mantenido por el servidor (volver a ejecutar tras subidas/descargas para refrescar la vista).
- `upload <fichero>`: lee un fichero local y lo copia al `FileManagerDir` remoto.
- `download <fichero>`: recupera un fichero remoto y lo guarda en el directorio local del cliente.
- `exit()`: termina la sesión del cliente.

## Secuencia de prueba recomendada

Ejemplo con **un servidor** y **dos clientes**:
1. Compila el proyecto y abre tres terminales dentro de `build/`.
2. Terminal 1: inicia el servidor en un puerto (p. ej., 5000):
   ```bash
   ./server 5000
   ```
3. Terminal 2: inicia el Cliente A:
   ```bash
   ./client 127.0.0.1 5000
   ```
4. Terminal 3: inicia el Cliente B:
   ```bash
   ./client 127.0.0.1 5000
   ```
5. En Cliente A:
   - Ejecuta `lls` para ver el estado inicial del `FileManagerDir` remoto.
   - Ejecuta `upload ejemplo.txt` (prepara previamente el archivo en el directorio local). Repite `lls` para confirmar que aparece en el servidor.
6. En Cliente B:
   - Ejecuta `lls` y comprueba que ahora ve `ejemplo.txt` (el directorio remoto es compartido por todas las sesiones del servidor).
   - Ejecuta `download ejemplo.txt` y verifica que el fichero se copia a su directorio local.
7. Repite `lls` tras cada `upload` o `download` para refrescar la lista de ficheros mostrada por el cliente.

## Uso del Broker (opcional)
Si implementas el broker de asignación de servidores:
1. Añade sus fuentes al `CMakeLists.txt` para generar el ejecutable `broker` y recompila.
2. Despliega la arquitectura con **1 Broker** y al menos **2 servidores** en máquinas/puertos distintos.
3. Lanza el broker, que escuchará peticiones de los clientes y responderá con la dirección de un servidor disponible.
4. Los clientes deben primero conectarse al broker para obtener IP/puerto de servidor y, a continuación, establecer la conexión directa con ese servidor.
5. Comportamiento esperado: reparto de carga entre servidores y posible divergencia de contenidos entre los distintos `FileManagerDir` (cada servidor mantiene su propia copia). Coordina los tests teniendo esto en cuenta.

## Ficheros proporcionados por la universidad
- `main_fm.cpp` (no modificable)
- `filemanager.h` (no modificable)
- `libFileManager.a` (no modificable)
- Proyecto base de CMake (`CMakeLists.txt` inicial)

## Ficheros a implementar por el estudiantado
- `client/fileManagerRemoto.cpp`
- `client/clientManager.h` y `client/clientManager.cpp` (lógica del cliente)
- `server/server.cpp`
- `server/clientManager.h` y `server/clientManager.cpp` (gestión de clientes en el servidor)
- `client/utils.cpp`, `client/utils.h`, `server/utils.cpp`, `server/utils.h`
- Cualquier fichero auxiliar adicional que necesites (recuerda añadirlo al CMake si procede)

## Problemas y soluciones frecuentes
- **Errores de socket/`connection refused`**: comprueba que el servidor está en ejecución y que usas la IP/puerto correctos.
- **Puerto en uso (`Address already in use`)**: cambia a otro puerto libre o libera el puerto cerrando procesos previos.
- **Servidor no alcanzable**: valida la conectividad de red y que no haya cortafuegos bloqueando el puerto.
- **Permisos en `FileManagerDir`**: asegúrate de que el usuario tiene permisos de lectura/escritura en el directorio de trabajo del servidor.
- **Lista desactualizada**: ejecuta `lls` nuevamente después de cada `upload` o `download` para refrescar la lista de ficheros.

## Cómo entregar la práctica
1. Empaqueta los fuentes y el `CMakeLists.txt` en un ZIP llamado `P2SISDISD_Nombre_Apellido.zip` (sustituye por tu nombre y apellido).
2. No incluyas carpetas binarias o de compilación (`bin/`, `build/`, `cmake-build-*`, etc.).
3. Añade un PDF o DOCX con capturas de pantalla que demuestren el funcionamiento solicitado.
4. Sube el ZIP siguiendo las indicaciones de tu campus virtual.

---
¡Éxitos con la implementación!
