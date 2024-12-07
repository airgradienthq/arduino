#include "LocalServer.h"

LocalServer::LocalServer(Stream &log, OpenMetrics &openMetrics,
                         Measurements &measure, Configuration &config,
                         WifiConnector &wifiConnector)
    : PrintLog(log, "LocalServer"), openMetrics(openMetrics), measure(measure),
      config(config), wifiConnector(wifiConnector) {}

LocalServer::~LocalServer() {}

bool LocalServer::begin(void) {
  server.on("/", HTTP_GET, [this]() { _GET_root(); });
  server.on("/measures/current", HTTP_GET, [this]() { _GET_measure(); });
  server.on(openMetrics.getApi(), HTTP_GET, [this]() { _GET_metrics(); });
  server.on("/config", HTTP_GET, [this]() { _GET_config(); });
  server.on("/config", HTTP_PUT, [this]() { _PUT_config(); });
  server.on("/dashboard", HTTP_GET, [this]() { _GET_dashboard(); });
  server.on("/storage/download", HTTP_GET, [this]() { _GET_storage(); });
  server.on("/storage/reset", HTTP_POST, [this]() { _POST_storage(); });
  server.on("/timestamp", HTTP_POST, [this]() { _POST_time(); });

  server.begin();

  if (xTaskCreate(
          [](void *param) {
            LocalServer *localServer = (LocalServer *)param;
            for (;;) {
              localServer->_handle();
            }
          },
          "webserver", 1024 * 4, this, 5, NULL) != pdTRUE) {
    Serial.println("Create task handle webserver failed");
  }
  logInfo("Init: " + getHostname() + ".local");

  return true;
}

void LocalServer::setAirGraident(AirGradient *ag) { this->ag = ag; }

String LocalServer::getHostname(void) {
  return "airgradient_" + ag->deviceId();
}

void LocalServer::_handle(void) { server.handleClient(); }

void LocalServer::_GET_root(void) {
  String body = "If you are not redirected automatically, go to <a "
                "href='http://192.168.4.1/dashboard'>dashboard</a>.";

  server.send(302, "text/html", htmlResponse(body, true));
}

void LocalServer::_GET_config(void) {
  if(ag->isOne()) {
    server.send(200, "application/json", config.toString());
  } else {
    server.send(200, "application/json", config.toString(fwMode));
  }
}

void LocalServer::_PUT_config(void) {
  String data = server.arg(0);
  String response = "";
  int statusCode = 400; // Status code for data invalid
  if (config.parse(data, true)) {
    statusCode = 200;
    response = "Success";
  } else {
    response = config.getFailedMesage();
  }
  server.send(statusCode, "text/plain", response);
}

void LocalServer::_GET_metrics(void) {
  server.send(200, openMetrics.getApiContentType(), openMetrics.getPayload());
}

void LocalServer::_GET_measure(void) {
  String toSend = measure.toString(true, fwMode, wifiConnector.RSSI(), *ag, config);
  server.send(200, "application/json", toSend);
}

void LocalServer::_GET_dashboard(void) {
  String timestamp = ag->getCurrentTime();
  server.send(200, "text/html", htmlDashboard(timestamp));
}

void LocalServer::_GET_storage(void) {
  char *data = measure.getLocalStorage();
  if (data != nullptr) {
    String filename =
        "measurements-" + ag->deviceId().substring(8) + ".csv"; // measurements-fdsa.csv
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    server.send(200, "text/plain", data);
    free(data);
  } else {
    server.send(204, "text/plain", "No data");
  }
}

void LocalServer::_POST_storage(void) {
  String body;
  int statusCode = 200;

  if (measure.resetLocalStorage()) {
    body = "Success reset storage";
  } else {
    body = "Failed reset local storage, unknown error";
    statusCode = 500;
  }
  body += ". Go to <a href='http://192.168.4.1/dashboard'>dashboard</a>.";

  server.send(statusCode, "text/html", htmlResponse(body, false));
}

void LocalServer::_POST_time(void) {
  String epochTime = server.arg(0);
  Serial.printf("Received epoch: %s \n", epochTime.c_str());
  if (epochTime.isEmpty()) {
    server.send(400, "text/plain", "Time query not provided");
    return;
  }

  long _epochTime = epochTime.toInt();
  if (_epochTime == 0) {
    server.send(400, "text/plain", "Time format is not in epoch time");
    return;
  }

  ag->setCurrentTime(_epochTime);

  String body = "Success set new time. Go to <a href='http://192.168.4.1/dashboard'>dashboard</a>.";
  server.send(200, "text/html", htmlResponse(body, false));
}

void LocalServer::setFwMode(AgFirmwareMode fwMode) { this->fwMode = fwMode; }

String LocalServer::htmlDashboard(String timestamp) {
  String page = "";
  page += "<!DOCTYPE html>";
  page += "<html lang=\"en\">";
  page += "<head>";
  page += "    <meta charset=\"UTF-8\">";
  page += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  page += "    <title>AirGradient Local Storage Mode</title>";
  page += "    <style>";
  page += "        body {";
  page += "            font-family: Arial, sans-serif;";
  page += "            display: flex;";
  page += "            flex-direction: column;";
  page += "            align-items: center;";
  page += "            margin-top: 50px;";
  page += "        }";
  page += "";
  page += "        button {";
  page += "            display: block;";
  page += "            margin: 10px 0;";
  page += "            padding: 10px 20px;";
  page += "            font-size: 16px;";
  page += "            cursor: pointer;";
  page += "        }";
  page += "        .datetime-container {";
  page += "            display: flex;";
  page += "            align-items: center;";
  page += "            margin: 10px 0;";
  page += "        }";
  page += "        .datetime-container input[type=\"datetime-local\"] {";
  page += "            margin-left: 10px;";
  page += "            padding: 5px;";
  page += "            font-size: 14px;";
  page += "        }";
  page += "        button.reset-button {";
  page += "            background-color: red;";
  page += "            color: white;";
  page += "            border: none;";
  page += "            padding: 10px 20px;";
  page += "            font-size: 16px;";
  page += "            cursor: pointer;";
  page += "        }";
  page += "        .spacer {";
  page += "            height: 50px;";
  page += "        }";
  page += "    </style>";
  page += "</head>";
  page += "<body>";
  page += "    <h2>";
  page += "    Device Time: ";
  page += timestamp;
  page += "    </h2>";
  page += "    <h2>";
  page += "    Serial Number: ";
  page += ag->deviceId();
  page += "    </h2>";
  page += "    <form action=\"/storage/download\" method=\"GET\">";
  page += "        <button type=\"submit\">Download Measurements</button>";
  page += "    </form>";
  page += "    <form id=\"timestampForm\" method=\"POST\" action=\"/timestamp\">";
  page += "        <input type=\"datetime-local\" id=\"timestampInput\" required>";
  page += "        <button type=\"submit\">Set Timestamp</button>";
  page += "        <input type=\"hidden\" name=\"timestamp\" id=\"epochInput\">";
  page += "    </form>";
  page += "    <div class=\"spacer\"></div>";
  page += "    <form action=\"/storage/reset\" method=\"POST\"";
  page += "        onsubmit=\"return confirm('Are you sure you want to reset the measurements? "
          "This action will permanently delete the existing measurement files!');\">";
  page += "        <button class=\"reset-button\" type=\"submit\">Reset Measurements</button>";
  page += "    </form>";
  page += "</body>";
  page += "<script>";
  page += "    document.querySelector('#timestampForm').onsubmit = function (event) {";
  page += "        const datetimeInput = document.querySelector('#timestampInput').value;";
  page += "        const localDate = new Date(datetimeInput);";
  page += "        const epochTimeUTC = Math.floor(Date.UTC(";
  page += "            localDate.getFullYear(),";
  page += "            localDate.getMonth(),";
  page += "            localDate.getDate(),";
  page += "            localDate.getHours(),";
  page += "            localDate.getMinutes()";
  page += "        ) / 1000);";
  page += "        document.querySelector('#epochInput').value = epochTimeUTC;";
  page += "        return true;";
  page += "    };";
  page += "</script>";
  page += "</html>";

  return page;
}

String LocalServer::htmlResponse(String body, bool redirect) {
  String page = "";
  page += "<!DOCTYPE HTML>";
  page += "<html lang=\"en-US\">";
  page += "    <head>";
  page += "        <meta charset=\"UTF-8\">";

  if (redirect) {
    page += "        <meta http-equiv=\"refresh\" content=\"0;url=/dashboard\">";
  }

  page += "        <title>Page Redirection</title>";
  page += "    </head>";
  page += "    <body>";
  page += body;
  page += "    </body>";
  page += "</html>";

  return page;
}