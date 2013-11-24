function MQTT(){
    var self = this;
    this.mqttClient = undefined;
    this.reconnect = function(){
      document.getElementById("log").innerHTML += "Reconnecting ...</br>"
      self.disconnect();
      self.connect(); 
    };
    this.connect = function(){
      self.mqttClient = new Messaging.Client(self.getValue(document.getElementById("host")), Number(self.getValue(document.getElementById("port"))), "WSS-client-" + Math.random().toString(36).substring(6));
      self.mqttClient.onConnectionLost = self.connectionLost;
      self.mqttClient.onMessageArrived = self.messageArrived;
      self.mqttClient.connect({onSuccess:self.connected});
    };
    this.connected = function(response){
      document.getElementById("log").innerHTML += "Connected</br>"
      window.onbeforeunload = function(){self.disconnect()};
    };
    this.disconnect = function() {
      self.mqttClient.disconnect(); 
    };
    this.disconnected = function() {
      document.getElementById("log").innerHTML += "Disconnected</br>"     
    };
    this.connectionLost = function(response){ 
      if (response.errorCode !== 0) {
        document.getElementById("log").innerHTML += "Connection Lost. Trying to reconnect in a while ...</br>"
        setTimeout(function () {self.connect();}, 1000); // Schedule reconnect if this was not a planned disconnect
      }
      self.disconnected();
    };
    this.messageArrived = function(message){
      document.getElementById("log").innerHTML += "Topic: " + message.destinationName + " Payload: " + message.payloadString + "</br>"

    };
    this.subscribe = function(){
        self.mqttClient.subscribe(self.getValue(document.getElementById("subscription_topic")), {"onSuccess" : function(){document.getElementById("log").innerHTML += "Subscription successful</br>"}});
    };
    this.unsubscribe = function(){
        self.mqttClient.unsubscribe(self.getValue(document.getElementById("unsubscription_topic")), {"onSuccess" : function(){document.getElementById("log").innerHTML += "Unsubscription successful</br>"}});
    };
    this.publish = function(){
      var message = new Messaging.Message(self.getValue(document.getElementById("message")));
      message.destinationName = self.getValue(document.getElementById("publish_topic"));
      this.mqttClient.send(message); 
    };
    this.getValue = function(element){
        if(element.value == "")
            return element.placeholder;
        else
            return element.value;
    };
};

mqtt = new MQTT();

window.onerror = function myErrorHandler(errorMsg, url, lineNumber) {
    document.getElementById("log").innerHTML += errorMsg + " See console for more information.</br>";
    return false;
}

