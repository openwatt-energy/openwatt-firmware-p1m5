#include <Arduino.h>
#include "web_ui.h"
#include "config.h"
#include "serial_console.h"
#include "generated/templates.h"

// Helper to get logo (same for all)
String getLogo() {
  String logo = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 48 48\" class=\"h-10 w-10\">";
  logo += "<defs><linearGradient id=\"owGrad\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"><stop offset=\"0%\" style=\"stop-color:#3b82f6;stop-opacity:1\" /><stop offset=\"100%\" style=\"stop-color:#1d4ed8;stop-opacity:1\" /></linearGradient></defs>";
  logo += "<circle cx=\"24\" cy=\"24\" r=\"22\" fill=\"url(#owGrad)\" />";
  logo += "<path fill=\"white\" d=\"M28 12l-12 16h8l-4 16 16-20h-8l4-12z\" />";
  logo += "</svg>";
  return logo;
}

String getWebPage(const String& path) {
  SerialConsole::println("WEB: Requesting path: " + path);
  String page;
  String logo = getLogo();

  if (path == "/" || path == "/index.html") {
    page = String(TEMPLATE_DASHBOARD);
  } else if (path == "/settings" || path == "/settings.html") {
    page = String(TEMPLATE_SETTINGS);
  } else if (path == "/system" || path == "/system.html") {
    page = String(TEMPLATE_SYSTEM);
  } else {
    SerialConsole::println("WEB: Path not found: " + path);
    return "";
  }

  page.replace("{{LOGO}}", logo);
  page.replace("{{CUSTOMER_NAME}}", CUSTOMER_DISPLAY_NAME);

  return page;
}
