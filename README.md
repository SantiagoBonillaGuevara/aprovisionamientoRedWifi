# ESP32 - Aprovisionamiento de Red WiFi
## Integrantes de trabajo
- Danna Alejandra Sanchez Monsalve

Este proyecto implementa un **captive portal en un ESP32** que permite aprovisionar la conexión WiFi de manera sencilla.  
El dispositivo levanta un **Access Point (AP)** y un **servidor HTTP**, mostrando una página web de configuración donde el usuario puede:

- Escanear redes disponibles.
- Seleccionar un SSID.
- Enviar credenciales (SSID, contraseña).
- Guardarlas en memoria no volátil.
- Conectarse automáticamente a la red indicada.
- Consultar el estado de la conexión o borrar credenciales.

---

## 📝 Explicación breve del código

- Se usa la librería **WiFi.h** para manejar la conexión a redes.
- **WebServer** levanta un servidor HTTP en el puerto 80.
- **DNSServer** redirige todo tráfico al ESP32 (captive portal).
- **Preferences** guarda de forma persistente las credenciales WiFi (`ssid` y `pass`).
- Al final de la ejucion del codigo se imprime la ip asignada al ESP32
- El flujo de arranque es:
  1. Revisar si existen credenciales guardadas.
  2. Intentar conexión a la red.
  3. Si falla, crear un AP abierto llamado `ESP32_Config`.
  4. Mostrar página web de configuración en `http://[IP_ESP32]/`.

---

## Endpoints de la API

### `GET /scan`
- Escanea las redes WiFi disponibles y devuelve un array JSON con los SSID.  
- **Ejemplo de respuesta:**
  ```json
  [
    "Red1",
    "Red2",
    "Red3"
  ]
### `POST /connect`
- Intenta conectar a la red usando las credenciales recibidas en formato JSON.

- Guarda credenciales si la conexión es exitosa.

Request body:

  ```json
  {
    "ssid": "MiRed",
    "pass": "mi_contraseña"
  }
```
## Respuestas:

#### Éxito:

```json
{
  "success": true,
  "ip": "172.20.10.2"
}
```
#### Error:

```json
{
  "success": false,
  "message": "No se pudo conectar con esas credenciales"
}
```
### `GET /status`

- Devuelve el estado actual de la conexión.

- **Ejemplo de respuesta:**

```json
{
  "connected": true,
  "ssid": "MiRed",
  "ip": "172.20.10.2"
}
```
### `GET /forget`

- Borra las credenciales guardadas y reinicia el dispositivo.

- **Ejemplo de respuesta:**

```Credenciales borradas. Reiniciando...```


## Diagramas

### Diagrama de componentes

![diagramaDeComponentes](https://github.com/user-attachments/assets/39a2c1c7-9cd6-4ef2-90fa-6c2d14a8b0ae)

### Diagrama de estados

![DiagramaDeEstados](https://github.com/user-attachments/assets/e72f9932-9ffa-4ffb-b51e-0e7829f06a92)

### Diagrama de secuencias

![diagramaUmlDeSecuencia](https://github.com/user-attachments/assets/42fe364e-a9fb-4d18-8f88-087d5750e6ec)

## Conversaciones con ChatGPT
Parte de la documentación y el diseño de la API fueron asistidos con ChatGPT.
[Ir a conversacion con chatGPT](https://chatgpt.com/share/68dd807a-ae50-8011-96c7-04e7e8ea28c2)

## GitHub Pages
Se puede desplegar la documentación de la API (OpenAPI/Swagger UI) en GitHub Pages para visualización pública.
[Ir a GitHub Pages](https://santiagobonillaguevara.github.io/aprovisionamientoRedWifi/)

## Recursos
Código fuente: Aprovisionamiento_de_red_WiFi.ino

Colección Postman: ESP32-Aprovicionamiento_de_red_WIFI.postman_collection.json

OpenAPI Spec: docs/openAPI.yaml

