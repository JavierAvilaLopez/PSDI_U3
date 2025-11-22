# Capa RPC FileManager (Resumen)

## Protocolo
- **Frame**: `uint32_be length` (opcode + payload) seguido de `uint8 opcode` y `payload` binario.
- **Opcodes**: `CTOR(2)`, `LIST(3)`, `UPLOAD(4)`, `DOWNLOAD(5)`, `BYE(6)`, `ERR(255)`.
- **Límites**: `length <= 16 MiB` (configurado en `rpc_utils.h`).
- **Códigos**: respuestas usan `status(uint32)` donde `0=OK`, `1=NOT_READY`, `2=BAD_OPCODE`, `3=EXCEPTION`.
- **Payloads**:
  - `CTOR`: string de ruta. Respuesta `status`.
  - `LIST`: sin payload. Respuesta `count(uint32) + strings`.
  - `UPLOAD`: `name(str) + size(uint32) + bytes`. Respuesta `status`.
  - `DOWNLOAD`: `name(str)`. Respuesta `status + size + bytes` si `status==0`.
  - `BYE`: sin payload; cierre ordenado.

## Flujo
1. Cliente instancia `FileManager` remoto (`fileManagerRemoto.cpp`), abre `RpcChannel` TCP (`clientManager.*`).
2. Envía `CTOR(dir)`; el servidor crea `FileManager` real (`libFileManager.a`).
3. Cada método (`listFiles`, `writeFile`, `readFile`) serializa parámetros y espera respuesta homóloga.
4. Servidor (`server.cpp`) atiende con un hilo por conexión; valida longitudes y captura excepciones.

## Observabilidad
- Macros `LOG_{DEBUG,INFO,WARN,ERR}` con prefijo `[ts][pid][tid][role][conn][op]`.
- Activación por `FILEMGR_LOG=debug` o `--verbose`; opción `--log-file` para volcar a fichero.
- Trazas en conexión, framing (`read/write` reintentos), envíos/recepciones y latencias de RPC.

## Seguridad
- Validación de tamaño antes de reservar memoria (`RPC_MAX_PAYLOAD`).
- Serialización explícita `uint32_be` sin dependencias externas.
- Cierre ordenado (`BYE`) y manejo de errores `ERR`.

