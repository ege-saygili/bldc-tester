#include "webserver.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setupWebServer() {
    ws.onEvent([](AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type,
                 void * arg, uint8_t *data, size_t len) {
        if(type == WS_EVT_DATA) {
            AwsFrameInfo * info = (AwsFrameInfo*)arg;
            if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                data[len] = 0;
                handleWebSocketMessage(client, (char*)data);
            }
        }
    });
    
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        const char* html = R"html(
<!DOCTYPE html>
<html>
<head>
    <title>BLDC Motor Tester</title>
    <style>
        body { font-family: Arial; margin: 20px; background-color: #f5f5f5; }
        .card { background: white; padding: 20px; margin: 10px; border-radius: 8px; 
                box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .status-ok { color: green; font-weight: bold; }
        .status-error { color: red; font-weight: bold; }
        .parameter-label { display: inline-block; width: 200px; font-weight: bold; }
        button { padding: 10px 20px; margin: 5px; border-radius: 4px; 
                 background-color: #007bff; color: white; border: none; cursor: pointer; }
        button:hover { background-color: #0056b3; }
        input[type="range"] { width: 200px; }
    </style>
</head>
<body>
    <h1>BLDC Motor Tester</h1>

    <div class="card">
        <h2>Motor Status</h2>
        <p><span class="parameter-label">Supply Voltage:</span> 
           <span id="supply-voltage">--</span> V</p>
        <p>
            <span class="parameter-label">Voltage Limit:</span>
            <input type="number" id="voltage-limit" min="0" max="24" step="0.1" value="12">
            <button onclick="updateVoltageLimit()">Set</button>
        </p>
        <p>
            <span class="parameter-label">Direction:</span>
            <select id="motor-direction">
                <option value="CW">Clockwise</option>
                <option value="CCW">Counter-Clockwise</option>
            </select>
            <button onclick="updateDirection()">Set</button>
        </p>
        <p><span class="parameter-label">Motor Pole Pairs:</span> 
           <span id="pole-pairs">--</span></p>
        <p><span class="parameter-label">Motor KV:</span> 
           <span id="motor-kv">--</span> RPM/V</p>
    </div>

    <div class="card">
        <h2>Phase Parameters</h2>
        <h3>Phase A</h3>
        <p><span class="parameter-label">Status:</span> 
           <span class="status-ok" id="phaseA-status">--</span></p>
        <p><span class="parameter-label">Resistance:</span> 
           <span id="phaseA-resistance">--</span> Ω</p>
        <p><span class="parameter-label">Inductance:</span> 
           <span id="phaseA-inductance">--</span> mH</p>

        <h3>Phase B</h3>
        <p><span class="parameter-label">Status:</span> 
           <span class="status-ok" id="phaseB-status">--</span></p>
        <p><span class="parameter-label">Resistance:</span> 
           <span id="phaseB-resistance">--</span> Ω</p>
        <p><span class="parameter-label">Inductance:</span> 
           <span id="phaseB-inductance">--</span> mH</p>

        <h3>Phase C</h3>
        <p><span class="parameter-label">Status:</span> 
           <span class="status-ok" id="phaseC-status">--</span></p>
        <p><span class="parameter-label">Resistance:</span> 
           <span id="phaseC-resistance">--</span> Ω</p>
        <p><span class="parameter-label">Inductance:</span> 
           <span id="phaseC-inductance">--</span> mH</p>
    </div>

    <div class="card">
        <h2>Hall Sensor Status</h2>
        <p><span class="parameter-label">Hall A:</span> 
           <span class="status-ok" id="hallA-status">--</span></p>
        <p><span class="parameter-label">Hall B:</span> 
           <span class="status-ok" id="hallB-status">--</span></p>
        <p><span class="parameter-label">Hall C:</span> 
           <span class="status-ok" id="hallC-status">--</span></p>
    </div>

    <div class="card">
        <h2>Test Section</h2>
        <button onclick="startTest()">Run Test</button>
        <button onclick="stopTest()">Stop Test</button>
        <p>
            <span class="parameter-label">Duty Cycle:</span>
            <input type="range" id="duty-cycle" min="0" max="100" value="50">
            <span id="duty-cycle-value">50</span>%
        </p>
    </div>

    <script>
        var ws = new WebSocket('ws://' + window.location.hostname + '/ws');
        
        ws.onmessage = function(event) {
            var data = JSON.parse(event.data);
            updateUI(data);
        };

        function updateUI(data) {
            // Update all UI elements with received data
            if(data.voltage) document.getElementById('supply-voltage').textContent = data.voltage;
            if(data.polePairs) document.getElementById('pole-pairs').textContent = data.polePairs;
            if(data.motorKv) document.getElementById('motor-kv').textContent = data.motorKv;
            if(data.voltageLimit) {
                document.getElementById('voltage-limit').value = data.voltageLimit;
            }
            if(data.direction) {
                document.getElementById('motor-direction').value = data.direction;
            }
            // ... update other elements
        }

        function startTest() {
            ws.send(JSON.stringify({command: 'start'}));
        }

        function stopTest() {
            ws.send(JSON.stringify({command: 'stop'}));
        }

        // Update duty cycle value display and send to server
        document.getElementById('duty-cycle').oninput = function() {
            var value = this.value;
            document.getElementById('duty-cycle-value').textContent = value;
            ws.send(JSON.stringify({command: 'duty', value: value}));
        };

        function updateVoltageLimit() {
            const limit = document.getElementById('voltage-limit').value;
            ws.send(JSON.stringify({
                command: 'setVoltageLimit',
                value: parseFloat(limit)
            }));
        }

        function updateDirection() {
            const direction = document.getElementById('motor-direction').value;
            ws.send(JSON.stringify({
                command: 'setDirection',
                value: direction
            }));
        }
    </script>
</body>
</html>
        )html";
        request->send(200, "text/html", html);
    });

    server.begin();
}

void handleWebSocketMessage(AsyncWebSocketClient *client, const char *message) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.println("Failed to parse WebSocket message");
        return;
    }

    const char* command = doc["command"];
    
    if (strcmp(command, "setVoltageLimit") == 0) {
        float limit = doc["value"];
        motor.voltage_limit = limit;
        // Broadcast back the new setting
        StaticJsonDocument<200> response;
        response["voltageLimit"] = motor.voltage_limit;
        String jsonString;
        serializeJson(response, jsonString);
        broadcastJson(jsonString);
    } 
    else if (strcmp(command, "setDirection") == 0) {
        const char* dir = doc["value"];
        motor.sensor_direction = (strcmp(dir, "CW") == 0) ? Direction::CW : Direction::CCW;
        // Broadcast back the new setting
        StaticJsonDocument<200> response;
        response["direction"] = (motor.sensor_direction == Direction::CW) ? "CW" : "CCW";
        String jsonString;
        serializeJson(response, jsonString);
        broadcastJson(jsonString);
    }
    else if (strcmp(command, "start") == 0) {
        // Handle start test command
    } else if (strcmp(command, "stop") == 0) {
        // Handle stop test command
    } else if (strcmp(command, "duty") == 0) {
        float duty = doc["value"];
        // Handle duty cycle change
    }
}

void broadcastJson(const String& json) {
    ws.textAll(json);
}