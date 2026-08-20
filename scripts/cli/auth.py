import hashlib
import base64

def generate_password(device_id: str, secret_key: str) -> str:
    """Generate the MQTT password based on secret key and device ID."""
    combined = secret_key + device_id
    sha256 = hashlib.sha256(combined.encode()).digest()
    pwd_bytes = sha256[:10]
    return base64.b64encode(pwd_bytes).decode()
