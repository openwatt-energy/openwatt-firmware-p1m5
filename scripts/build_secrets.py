"""Inject build-time secrets from environment variables.

Reads optional MQTT_SALT and MQTT_BROKER_HOST from the environment and appends
them as preprocessor defines. Values are only applied when the environment
variable is set, so the defaults in src/config.h are used otherwise. The secret
values never live in the repository.

Usage:
    MQTT_SALT="<secret>" MQTT_BROKER_HOST="mqtt.openwatt.eu" pio run -e openwatt
"""

import os

Import("env")

SECRETS = {
    "MQTT_SALT": "SALT_STRING",
    "MQTT_BROKER_HOST": "MQTT_BROKER_HOST",
}

for env_var, macro in SECRETS.items():
    value = os.environ.get(env_var, "").strip()
    if value:
        env.Append(BUILD_FLAGS=['-D{}=\\"{}\\"'.format(macro, value)])
        print("build_secrets: {} -> {}={!r}".format(env_var, macro, value))
