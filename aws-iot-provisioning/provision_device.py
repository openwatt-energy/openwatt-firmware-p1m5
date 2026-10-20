import os
import json
import argparse
import requests
import boto3
from dotenv import load_dotenv

load_dotenv()

ENV_PARAMETERS = {
    "prod": {
        "policy_name": "Soliseco-P1-Policy-Prod",
        "django_provisioning_url": "https://admin.my.soliseco.com/api-v1/device/provision/",
    },
    "staging": {
        "policy_name": "Soliseco-P1-Policy-Staging",
        "django_provisioning_url": "https://admin.my-staging.soliseco.com/api-v1/device/provision/",
    },
    "dev": {
        "policy_name": "Soliseco-P1-Policy-Staging",
        "django_provisioning_url": "http://localhost:8000/api-v1/device/provision/",
    }
}

DEFAULT_ROOT_CA_URL = "https://www.amazontrust.com/repository/AmazonRootCA1.pem"

parser = argparse.ArgumentParser(
    prog="IoT Core Provisioning",
    description="Provision device on AWS IoTCore"
)
parser.add_argument('--thing-name', dest="thing_name", required=True)
parser.add_argument('--env', dest="env", choices=list(ENV_PARAMETERS.keys()), required=True)

parser.add_argument('--output-dir', dest="output_dir", default="./")
parser.add_argument('--local-save', dest="local_save", action=argparse.BooleanOptionalAction, default=False)
parser.add_argument('--django-provisioning', dest="django_provisioning", action=argparse.BooleanOptionalAction, default=True)
parser.add_argument('--root-ca-url', dest="root_ca_url", default=DEFAULT_ROOT_CA_URL)

args = parser.parse_args()

THING_NAME = args.thing_name
ENV = args.env
OUTPUT_DIR = args.output_dir
LOCAL_SAVE = args.local_save
DJANGO_PROVISIONING = args.django_provisioning
ROOT_CA_URL = args.root_ca_url

POLICY_NAME = ENV_PARAMETERS[ENV]['policy_name']
DJANGO_PROVISIONING_URL = ENV_PARAMETERS[ENV]['django_provisioning_url']

session = boto3.Session(region_name='eu-central-1')

iot_client = session.client('iot')
s3 = session.resource('s3')


def create_thing(thing_name, policy_name, env):
    try:
        # Check if thing already exists
        iot_client.describe_thing(thingName=thing_name)
    except iot_client.exceptions.ResourceNotFoundException:
        # Create thing corresponding to sensor
        iot_client.create_thing(
            thingName=thing_name,
            thingTypeName="P1",
            attributePayload={
                "attributes": {"env": env}
            }
        )
    else:
        raise Exception('Thing already exists')

    cert_and_keys = iot_client.create_keys_and_certificate(setAsActive=True)
    certificate_arn = cert_and_keys['certificateArn']
    certificate = cert_and_keys['certificatePem']
    pub_key = cert_and_keys['keyPair']['PublicKey']
    private_key = cert_and_keys['keyPair']['PrivateKey']
    iot_client.attach_policy(policyName=policy_name, target=certificate_arn)
    iot_client.attach_thing_principal(thingName=thing_name, principal=certificate_arn)
    return certificate, private_key, pub_key


def provision_on_django(thing_name, certificate, private_key, pub_key):
    files = {
        'private_key': private_key,
        'public_key': pub_key,
        'cert': certificate
    }
    data = {
        "device_id": thing_name,
        "secret": os.environ['DEVICE_PROVISIONING_ACCESS_KEY']
    }
    resp = requests.post(
    DJANGO_PROVISIONING_URL,
    data=data,
    files=files,
    timeout=30
)

    resp.raise_for_status()
    return resp.json()

def save_to_fs(thing_name, output_dir, certificate, private_key, pub_key, amazon_root_CA1, endpoint_url):
    new_dir = os.path.join(output_dir, thing_name)
    if not os.path.exists(new_dir):
        os.mkdir(new_dir)
    amazon_root_ca_path = os.path.join(new_dir, 'AmazonRootCA1.pem')
    private_key_path = os.path.join(new_dir, thing_name + '.private.pem.key')
    pub_key_path = os.path.join(new_dir, thing_name + '.public.pem.key')
    certiface_path = os.path.join(new_dir, thing_name + '.pem.crt')
    endpoint_url_path = os.path.join(new_dir, thing_name + 'endpoint_url.json')

    with open(private_key_path, 'w') as file:
        file.write(private_key)

    with open(pub_key_path, 'w') as file:
        file.write(pub_key)

    with open(certiface_path, 'w') as file:
        file.write(certificate)

    with open(amazon_root_ca_path, 'w') as file:
        file.write(amazon_root_CA1)

    with open(endpoint_url_path, 'w') as file:
        file.write(endpoint_url)

def get_amazon_root_ca(url):
    resp = requests.get(url)
    return resp.text

def get_endpoint_url():
    endpoint = iot_client.describe_endpoint()
    endpoint_url = endpoint.get('endpointAddress')
    print(endpoint_url)
    return json.dumps({'url': endpoint_url}, indent=4)

certificate, private_key, pub_key = create_thing(THING_NAME, POLICY_NAME, ENV)

amazon_root_CA1 = get_amazon_root_ca(ROOT_CA_URL)

endpoint_url = get_endpoint_url()

if LOCAL_SAVE:
    save_to_fs(THING_NAME, OUTPUT_DIR, certificate, private_key, pub_key, amazon_root_CA1, endpoint_url)

if DJANGO_PROVISIONING:
    provision_on_django(THING_NAME, certificate, private_key, pub_key)
