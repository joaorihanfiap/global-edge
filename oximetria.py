import requests
import json

url = "http://46.17.108.113:1026/v2/entities/urn:ngsi-ld:triagem:001/attrs/oximetria"

payload = {}
headers = {
    'fiware-service': 'smart',
    'fiware-servicepath': '/',
    'accept': 'application/json'
}

response = requests.request("GET", url, headers=headers, data=payload)

if response.status_code == 200:
    data = json.loads(response.text)
    oximetria_value = data.get('value', None) 

    if oximetria_value is not None and oximetria_value < 95:
        print(f"\n\nAlerta, níveis de oxigênio muito baixos! ({oximetria_value}%)\n\n")
    else:
        print(f"\n\nO valor de oxigênio é de {oximetria_value}%\n\n")
else:
    print(f"Failed to fetch data. Status code: {response.status_code}")
