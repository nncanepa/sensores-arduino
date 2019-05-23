# Sensores nivel tierra

Programa para Arduino MEGA (con Ethernet Shield) que lee y publica
un JSON por MQTT los siguientes sensores:

*  Presiones de los 6 canales de un Pfeifer MaxiGauge.
*  Temperatura y humedad con un sensor DHT22.
*  Seteo y lectura de un flujímetroo Alicat.
*  Dos switches que indican flujo de refrigeración para blanco y fuente de iones.

Requiere las siguientes librerias:

*  ArduinoJson (https://arduinojson.org/).
*  Adafruit_Sensors y DHT.
*  PubSubClient (para MQTT).
*  Ethernet para el shield de arduino.

Topics en los que publica:
*  N_TIERRA/SENSORES/VACIO/read
*  N_TIERRA/SENSORES/TEMPHUM/read
*  N_TIERRA/SENSORES/FLUX/read
*  N_TIERRA/SENSORES/FLUX/rwrite
*  N_TIERRA/SENSORES/FLOW_SW/read

JSON MaxiGauge (mbar):
`{"getV1":0.02,"getV2":0.02,"getV3":0.02,"getV4":0.02,"getV5":0.02,"getV6":1000}`

JSON Temperatura (ºC) y humedad (%HR):
`{"getT":22.2,"getH":57}`

JSON Flujimetro (sccm):
`Seteo -> {"setV":5.0}`
`Lectura -> {"getV":0.1}`

JSON Flow switches:
`{"getS1":false,"getS2":false}`

