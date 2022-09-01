from re import I
import this
from django.shortcuts import render
from django.http import HttpResponse
import json
from Crypto import Random
from hashlib import sha512
from django.views.decorators.csrf import csrf_exempt
from Crypto.Signature import pkcs1_15
from Crypto.PublicKey import RSA
import random
from Crypto.Hash import SHA512, SHA384, SHA256, SHA, MD5
from Crypto.Signature import PKCS1_v1_5

global auth_requests
auth_requests = {}
#Set Path for keys
keypath = "/var/keys/"
#Global Variables for temp and humidity, and pubkey of client
global public_key
global humidity
global temperature
#base value for temp and hum
humidity = 10
temperature = 5

# Create your views here.
def index(request):
    var_text = "Welcome to the test site.\n"
    return render(request, 'index/index.html', {'var_text' : var_text})

#return the temperature and humidity
def requestVals(request):
    global temperaure
    global humidity
    print("requested")
    if request.method == 'GET':
        values = {
                "Temperature" : temperature, 
                "Humidity" : humidity
            }
        return HttpResponse(json.dumps(values, indent=4))
    else :
        return HttpResponse("You're not supposed to be here.")

#return a randomly generated challenge
def getAuth(request):
    print("getAuth")
    if request.method == 'GET':
        #determine user ip
        ip = ""
        x_forwarded_for = request.META.get('HTTP_X_FORWARDED_FOR')
        if x_forwarded_for:
            ip = x_forwarded_for.split(',')[0]
        else:
            ip = request.META.get('REMOTE_ADDR')
        
        #generate random key
        #The limit for the extended ASCII Character set
        MAX_LIMIT = 255
        
        random_string = ''
        
        for _ in range(10):
            random_integer = random.randint(0, MAX_LIMIT)
            #Keep appending random characters to randomstring
            random_string += (chr(random_integer))    
        
        #check if user already in keys
        global auth_requests
        #Save hash of randomstring to check for equality after challenge is completed
        auth_requests[ip] = sha512(random_string.encode('utf-8')).digest()
        
        return HttpResponse(random_string)

#check if the challenge was completed successfully, add the humidity and temperature to the global database
@csrf_exempt
def checkAuth(request):
    print("checkauth")
    global auth_requests
    #save json to load it with json library
    with open('/var/www/growfridge/logs/body.txt', 'w+') as f:
        f.write(request.body.decode('utf-8'))
    key = json.loads(request.body.decode('utf-8'))['key']
    loadedJson = json.loads(request.body.decode('utf-8'))

    #determine user ip
    ip = ""
    x_forwarded_for = request.META.get('HTTP_X_FORWARDED_FOR')
    if x_forwarded_for:
        ip = x_forwarded_for.split(',')[0]
    else:
        ip = request.META.get('REMOTE_ADDR')
    #load pubkey
    public_key = ""
    with open(keypath + "public.pem", "rb") as f:
        public_key = RSA.importKey(f.read())
    #check if user already in keys
    try:
        #if ip matches challenge, add new values
        if str(ip) in auth_requests:
            #verify if challenge was signed with privkey
            if verify(auth_requests[ip],key,  public_key):
                global temperature
                temperature = loadedJson["temperature"]
                global humidity
                humidity = loadedJson["humidity"]
                auth_requests.pop(ip)
                return HttpResponse("success")
            else:
                auth_requests.pop(ip)
                return HttpResponse("failure")
        #if the ip does not match the challenge, something is wrong
        else:
            return HttpResponse("failure")
    except Exception as e:
        print(e)
        print("faliure")

#helper function used to verify the challenge   
def verify(message, signature, pub_key):
    signer = PKCS1_v1_5.new(pub_key)
    digest = SHA512.new()
    digest.update(message)
    return signer.verify(digest, signature)

