# Script for IoT Core provisioning

## Setup

First activate the virtual env and install dependencies

```bash
pipenv shell
pipenv install # Only when Pipfile changes
```

Add a `.env` file in the root directory.

The `.env` file should contain the following variables:

```bash
AWS_ACCESS_KEY_ID=YOUR_ACCESS_KEY
AWS_SECRET_ACCESS_KEY=YOUR_SECERT_ACCESS_KEY
DEVICE_PROVISIONING_ACCESS_KEY=YOUR_DEVICE_PROVISIONING_ACCESS_KEY
```

The user associated to the credentials should have the following permissions for IoT Core

```json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "VisualEditor0",
            "Effect": "Allow",
            "Action": [
                "iot:CreateThing",
                "iot:AttachPolicy",
                "iot:AttachThingPrincipal",
                "iot:DescribeThing",
                "iot:CreateKeysAndCertificate",
                "iot:DescribeEndpoint"
            ],
            "Resource": "*"
        }
    ]
}
```

## How to use

You can run the script `provision_device.py` as follow:

```bash
python3 provision_device.py --thing-name YOUR_THING_NAME --uuid YOUR_DEVICE_UUID
```

To see all available script arguments execute:

```bash
python3 provision_device.py --help
```

## To be done

- Create device in database
