from flask import Flask, render_template_string, jsonify
import requests
from datetime import datetime
import threading
import logging

# Oculta el spam de peticiones GET cada segundo en la consola para no estorbar tu menú
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)

# CAMBIA ESTA IP POR LA DE TU ESP32
IP_ESP32 = "http://10.220.96.206"

historial_eventos = []

def ejecutar_orden(cmd, origen="Web"):
    try:
        res = requests.get(f"{IP_ESP32}/api?cmd={cmd}", timeout=1.5)
        estado_actual = res.text.strip()
        
        nombres = {'A': 'ARRANQUE', 'P': 'PARO', '+': 'ACELERAR (+)', '-': 'DESACELERAR (-)'}
        if cmd in nombres:
            hora = datetime.now().strftime("%H:%M:%S")
            msg = f"[{hora}] ({origen}) {nombres[cmd]} -> Estado: {estado_actual}"
            historial_eventos.insert(0, msg)
            if origen == "Consola":
                print(f"\n>>> {msg}")
        return estado_actual
    except:
        if origen == "Consola":
            print("\n>>> [ERROR] No hay conexión con la ESP32.")
        return "SIN CONEXIÓN ESP32"

HTML_INTERFAZ = """
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Control Motor 6.5 HP</title>
    <style>
        body { font-family: 'Segoe UI', sans-serif; background: #121212; color: #fff; text-align: center; margin: 0; padding: 15px; }
        .card { background: #1e1e1e; max-width: 450px; margin: auto; padding: 25px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0,0,0,0.6); }
        .led-indicador { width: 75px; height: 75px; border-radius: 50%; margin: 15px auto; background: #ffa000; box-shadow: 0 0 25px #ffa000; transition: 0.3s; }
        .btn { width: 100%; padding: 18px; font-size: 18px; font-weight: bold; border: none; border-radius: 10px; margin: 8px 0; cursor: pointer; }
        .btn-arrancar { background: #28a745; color: white; }
        .btn-paro { background: #dc3545; color: white; }
        .fila-acel { display: flex; justify-content: space-between; gap: 10px; margin-top: 8px; }
        .btn-acel { width: 48%; background: #007bff; color: white; padding: 16px; font-size: 16px; border: none; border-radius: 10px; cursor: pointer; font-weight: bold; }
        .log { text-align: left; background: #141414; padding: 10px; border-radius: 8px; height: 140px; overflow-y: auto; font-size: 12px; color: #bbb; margin-top: 15px; }
    </style>
</head>
<body>
    <div class="card">
        <h2>Control Motor 6.5 HP</h2>
        <div id="led" class="led-indicador"></div>
        <h3 id="texto-estado">Estado: ESPERA</h3>
        
        <button class="btn btn-arrancar" onclick="enviar('A')">INICIO / ARRANCAR</button>
        <button class="btn btn-paro" onclick="enviar('P')">PARO DE EMERGENCIA</button>

        <div class="fila-acel">
            <button class="btn-acel" onclick="enviar('-')">- DESACELERAR</button>
            <button class="btn-acel" onclick="enviar('+')">+ ACELERAR</button>
        </div>
        
        <div class="log" id="caja-log">Sistema listo...</div>
    </div>

    <script>
        function actualizarUI(data) {
            const led = document.getElementById('led');
            const texto = document.getElementById('texto-estado');
            texto.innerText = "Estado: " + data.estado;

            if (data.estado === "ESPERA") {
                led.style.background = "#ffa000"; led.style.boxShadow = "0 0 25px #ffa000";
            } else if (data.estado === "ARRANCANDO") {
                led.style.background = "#0050ff"; led.style.boxShadow = "0 0 25px #0050ff";
            } else if (data.estado === "ENCENDIDO") {
                led.style.background = "#00ff00"; led.style.boxShadow = "0 0 25px #00ff00";
            } else {
                led.style.background = "#ff0000"; led.style.boxShadow = "0 0 25px #ff0000";
            }

            if (data.logs) {
                document.getElementById('caja-log').innerHTML = data.logs.join("<br>");
            }
        }

        function enviar(cmd) {
            fetch('/accion/' + cmd)
                .then(r => r.json())
                .then(data => actualizarUI(data));
        }

        setInterval(() => { enviar('S'); }, 1200);
    </script>
</body>
</html>
"""

@app.route('/')
def inicio():
    return render_template_string(HTML_INTERFAZ)

@app.route('/accion/<cmd>')
def accion(cmd):
    estado = ejecutar_orden(cmd, origen="Web/Cel")
    return jsonify({"estado": estado, "logs": historial_eventos[:15]})

def menu_consola():
    while True:
        print("\n==========================================")
        print("   MENÚ DE CONTROL - MOTOR 6.5 HP")
        print("==========================================")
        print("  [1] INICIO / ARRANCAR MOTOR")
        print("  [2] PARO DE EMERGENCIA")
        print("  [3] + ACELERAR")
        print("  [4] - DESACELERAR")
        print("  [5] CONSULTAR ESTADO ACTUAL")
        print("==========================================")
        opcion = input("Elige una opción (1-5): ").strip()

        if opcion == '1':
            ejecutar_orden('A', origen="Consola")
        elif opcion == '2':
            ejecutar_orden('P', origen="Consola")
        elif opcion == '3':
            ejecutar_orden('+', origen="Consola")
        elif opcion == '4':
            ejecutar_orden('-', origen="Consola")
        elif opcion == '5':
            est = ejecutar_orden('S', origen="Consola")
            print(f"\n>>> Estado actual en la STM32: {est}")
        else:
            print("\n>>> Opción no válida, escribe un número del 1 al 5.")

if __name__ == '__main__':
    # Inicia el servidor web en segundo plano para dejar libre el menú en consola
    hilo_web = threading.Thread(target=lambda: app.run(host='0.0.0.0', port=5000, use_reloader=False))
    hilo_web.daemon = True
    hilo_web.start()

    print("\nServidor Web activo en http://localhost:5000 (y desde tu celular)")
    menu_consola()
