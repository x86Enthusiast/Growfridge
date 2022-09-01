#Copyright (C) 2022  Michel Macke
#
#This program is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <https://www.gnu.org/licenses/>.
import os
from Crypto import client.py.pydom
from hashlib import sha512
from Crypto.Signature import pkcs1_15
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
from Crypto.Signature import PKCS1_v1_5
from Crypto.Hash import SHA512, SHA384, SHA256, SHA, MD5
from base64 import b64encode, b64decode
import requests
import json
import fridge_monitor
import time

def sign(message, priv_key):
    signer = PKCS1_v1_5.new(priv_key)
    digest = SHA512.new()
    digest.update(message)
    return signer.sign(digest)

def main():
    fridge_monitor.Fridge_Monitor.start(arduino_con = '/dev/ttyUSB0', baudrate = 19200, timeout = 1)
    ip = "http://127.0.0.1:8000/"
    keypath = "/home/michel/growfridge/rpiclient/keys/"
    temperature = fridge_monitor.Fridge_Monitor.temperature
    humidity = fridge_monitor.Fridge_Monitor.humidity
    
    #read or generate private key
    if not os.path.isfile(keypath + "private.pem"):
        random_generator = Random.new().read
        private_key = RSA.generate(4096, random_generator)
        public_key = private_key.publickey()
        with open(keypath + "private.pem", "wb") as f:
            f.write(private_key.exportKey("PEM"))
        with open(keypath + "public.pem", "wb") as f:
            f.write(public_key.exportKey("PEM"))
        public_key.exportKey(format='PEM')
    public_key = ""
    private_key = ""
    with open(keypath + "public.pem", "rb") as f:
        public_key = RSA.importKey(f.read())
    with open(keypath + "private.pem", "rb") as f:
        private_key = RSA.importKey(f.read())
    
    
    while True:
        if fridge_monitor.Fridge_Monitor.temperature is not temperature or fridge_monitor.Fridge_Monitor.humidity is not humidity:
            temperature = fridge_monitor.Fridge_Monitor.temperature
            humidity = fridge_monitor.Fridge_Monitor.humidity
            print("temperature: " + temperature + " humidity: " + humidity)
            
            #do challenge response
            session = requests.Session()
            challenge = session.get("http://localhost:8000/getAuth/" )
            binaryContent = challenge.content.decode("utf-8")
            msg = binaryContent.encode('utf-8')
            print(challenge.content)
            signature = sign(msg, private_key)
            print(signature)

            signStr = int.from_bytes(signature, "big")
            session.put(url="http://localhost:8080/checkAuth/", data=json.dumps({"key": signStr, 'temperature' : temperature, 'humidity' : humidity}, indent=4))
        else:
            time.sleep(1)

if __name__ == "__main__":
    main()