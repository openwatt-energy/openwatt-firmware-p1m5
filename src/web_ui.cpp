#include "web_ui.h"
#include "config.h"
#include "generated/templates.h"
#include <WiFi.h>

// Helper to get logo (same for all)
String getLogo() {
  String logo = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 48 48\" class=\"h-10 w-10\">";
  logo += "<defs><linearGradient id=\"owGrad\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"><stop offset=\"0%\" style=\"stop-color:var(--primary);stop-opacity:1\" /><stop offset=\"100%\" style=\"stop-color:var(--accent);stop-opacity:1\" /></linearGradient></defs>";
  logo += "<circle cx=\"24\" cy=\"24\" r=\"22\" fill=\"url(#owGrad)\" />";
  logo += "<path fill=\"white\" d=\"M28 12l-12 16h8l-4 16 16-20h-8l4-12z\" />";
  logo += "</svg>";
  return logo;
}

String getThemeCSS() {
  String css = "<style>:root{--primary:" THEME_PRIMARY ";--bg:" THEME_BACKGROUND ";--text:" THEME_TEXT ";--accent:" THEME_ACCENT ";}</style>";
  return css;
}

String getWebPage(const String& path) {
  String page;

  if (path == "/" || path == "/index.html") {
    page = String(TEMPLATE_DASHBOARD);
  } else if (path == "/settings" || path == "/settings.html") {
    page = String(TEMPLATE_SETTINGS);
  } else if (path == "/system" || path == "/system.html") {
    page = String(TEMPLATE_SYSTEM);
  } else {
    return "";
  }

  page.replace("{{LOGO}}", getLogo());
  page.replace("{{CUSTOMER_NAME}}", CUSTOMER_DISPLAY_NAME);
  page.replace("{{THEME_CSS}}", getThemeCSS());

  return page;
}
