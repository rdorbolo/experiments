"use strict";

console.log("hello world");

var currenttime = new Date().getTime();
var lastTimeStamp = 0; // millsecs/10
var event = false;
var firstLoad = true;
var engineState = -1;
var newState  = 0;
var nextState = 0;


var bodyEl = document.getElementsByTagName("body")[0];
bodyEl.innerHTML = "";
var statusEl = document.createElement("div");
bodyEl.appendChild(statusEl);

var stateEl = document.createElement("div");
stateEl.classList.add("state");
stateEl.innerHTML = "No Data";

var buttonEl = document.createElement("div");
buttonEl.classList.add("button");
buttonEl.classList.add("buttonInactive");
buttonEl.innerHTML = "Unitialized";
buttonEl.onclick = onclickButton;


var timeEl         = document.createElement("div");
var altCntEl       = document.createElement("div");
var chargeStatusEl = document.createElement("div");
var avgDeltaEl     = document.createElement("div");
var pwm1El         = document.createElement("div");
var gpio35ValEl    = document.createElement("div");

statusEl.appendChild(stateEl);
statusEl.appendChild(buttonEl);

statusEl.appendChild(timeEl);
statusEl.appendChild(altCntEl);
statusEl.appendChild(chargeStatusEl);
statusEl.appendChild(avgDeltaEl);
statusEl.appendChild(pwm1El);
statusEl.appendChild(gpio35ValEl);

function Slider(name, min, max, initialVal, io) {

    this.min = min;
    this.max = max;

    this.sliderEl = document.createElement("input");
    this.sliderEl.setAttribute("type", "range");
    this.sliderEl.setAttribute("min", "" + min);
    this.sliderEl.setAttribute("max", "" + max);
    this.sliderEl.setAttribute("value", "" + initialVal);
    this.sliderEl.style.background = "rgba(255,0,0,.1)";
    this.sliderEl.me = this;

    this.sliderEl.classList.add("slider");

    this.sliderElCopy = document.createElement("input");
    this.sliderElCopy.setAttribute("type", "range");
    this.sliderElCopy.setAttribute("min", "" + min);
    this.sliderElCopy.setAttribute("max", "" + max);
    this.sliderElCopy.setAttribute("value", "" + initialVal);
    this.sliderElCopy.style.background = "rgba(255,127,0,.2)";
    this.sliderElCopy.classList.add("slider");
    this.sliderElCopy.disabled = true;

    if (io == "input") this.sliderElCopy.style.opacity = "0";

    if (io == "output") this.sliderEl.style.opacity = "0";
    if (io == "output") this.sliderEl.style.pointerEvents = "none";

    

    var sliderDivEl = document.createElement("div");
    this.name = name;

    if (name) {
        var nameEl = document.createElement("div");
        nameEl.innerText = name;
        sliderDivEl.appendChild(nameEl);
    }

    sliderDivEl.style.height = "3em";
    //sliderDivEl.style.position = "relative";

    sliderDivEl.appendChild(this.sliderElCopy);
    sliderDivEl.appendChild(this.sliderEl);

    this.el = sliderDivEl;


    Object.defineProperty(this, "value", {
        get: function () { return this.sliderEl.value },
        set: function (v) { this.sliderElCopy.value = v; this.updateText(); }
    });
    Object.defineProperty(this, "resetValue", {
        get: function () { return this.sliderEl.value },
        set: function (v) { this.sliderEl.value = v }
    });

    
    this.updateText = function () {
        if (this.textFunc)  nameEl.innerText = name + this.textFunc(this);
        else nameEl.innerText = name + " " + this.sliderEl.value + " / " + this.sliderElCopy.value;
    }



}


var pwmSlider    = new Slider("PWM1", 0, 512, 512);
var pwmSliderMin = new Slider("PWM1 Min Limit", 0, 512, 512, "input");
var pwmSliderMax = new Slider("PWM1 Max Limit", 0, 512, 512, "input");
var shunt0Slider = new Slider("Shunt0 (generator) Limit / Value", -2000, 30000, 0);
var shunt1Slider = new Slider("Shunt1 (Battery) NA / Value", -2000, 30000, 0, "output");

var adcTarget    = new Slider("gpio35 ADC Target", 0, 1023, 0, "input");
adcTarget.textFunc = (o) => {return " " + (12.6/620*o.sliderEl.value).toFixed(1) + " volts"}




statusEl.appendChild(pwmSlider.el);
statusEl.appendChild(pwmSliderMin.el);
statusEl.appendChild(pwmSliderMax.el)
statusEl.appendChild(shunt0Slider.el);
statusEl.appendChild(shunt1Slider.el);
statusEl.appendChild(adcTarget.el);


var voltageOuterEl = document.createElement("div");
voltageOuterEl.classList.add("voltageOuter");
var voltageEl = document.createElement("div");
voltageEl.classList.add("voltage");
var voltageElMinMax = document.createElement("div");
voltageElMinMax.classList.add("voltageMinMax");
var voltageElCenter = document.createElement("div");
voltageElCenter.classList.add("voltageCenter");
voltageEl.appendChild(voltageElMinMax);
voltageEl.appendChild(voltageElCenter);
voltageOuterEl.appendChild(voltageEl);
statusEl.appendChild(voltageOuterEl);






var lastInput;  // last Slider to move
statusEl.oninput = (e) => { event = true; lastInput=e.target.me; e.target.me.updateText(); /*console.log(e.srcElement.value)*/ }


var styleEl = document.createElement('style');
document.getElementsByTagName('head')[0].appendChild(styleEl);
styleEl.innerHTML = ".slider {width:80%; margin-left:10%; margin-right:10%; position:absolute;appearance: none; height: 1.5em; }"


var readyForData = true;

var lastTime = 0;
var avgDelta = 0;


var array = new Array();

var xmlhttp = new XMLHttpRequest();
var dataObj; 
xmlhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
        dataObj = JSON.parse(this.responseText);
        var ts = dataObj.ts;
        var tenths   = ts/10;
        var seconds = ts/100;
        var minutes = seconds/60;
        var hours   = minutes/60;

        hours   = Math.floor(hours);
        minutes = Math.floor(minutes - hours * 60)  ;
        seconds = Math.floor(seconds - hours*60*60 - minutes*60) ;
        tenths  = Math.floor(tenths  - hours*60*60*10 - minutes*60*10 - seconds*10);
    
        minutes = ("" + (1000 +  minutes)).substr(2,2)  ;
        seconds = ("" + (1000 +  seconds)).substr(2,2)  ;
        

       // timeEl.innerText = "timeStamp:" + Math.floor(hours)  + ":" + Math.floor(minutes) + ":" + Math.floor(seconds) /*+ (seconds - Math.floor(seconds))*/ +   " ("+ dataObj.ts + ")";
        timeEl.innerText = "timeStamp: " + hours + ":" + minutes + ":" + seconds + "." + tenths +   " ("+ dataObj.ts + ")";
        altCntEl.innerText = "AltCnt: " + dataObj.altCnt;
        pwm1El.innerText = "Pwm1:" + dataObj.pwm1;
        chargeStatusEl.innerText = "Status: " + dataObj.status;
        gpio35ValEl.innerText = "Battery Voltage (gpio35Val)  " + dataObj.gpio35Val + " = " + (12.6/620*dataObj.gpio35Val).toFixed(1) + "v";

        voltageElCenter.style.left = "" + (dataObj.gpio35Val / 1024 * 100 - .5) + "%"
        voltageElMinMax.style.left = "" + dataObj.gpio35ValMin / 1024 * 100 + "%"
        voltageElMinMax.style.right = "" + (100 - dataObj.gpio35ValMax / 1024 * 100) + "%"

        if (firstLoad || (dataObj.ts - lastTimeStamp) > 200) {
            firstLoad = false;
            pwmSlider.resetValue = dataObj.pwm1;
        }
        pwmSlider.value = dataObj.pwm1;
        shunt0Slider.value = dataObj.shunt0Sum/dataObj.shunt0Cnt;
        shunt1Slider.value = dataObj.shunt1;

        var stateTxt = "Undefined";
        engineState = dataObj.state;
        if (engineState == 1) {
            stateTxt = "Reset";
            stateEl.style.background = "gray";
            buttonEl.innerText = "Move to Ready"
            buttonEl.classList.remove("buttonInactive");
            nextState          = 2;
        }
        else if (engineState == 2) {
            stateTxt = "Ready";
            stateEl.style.background = "yellow";
            buttonEl.innerText = "Move to Run"
            buttonEl.classList.remove("buttonInactive");
            nextState          = 3;
        }
        else if (engineState == 3) {
            stateTxt = "running"
            stateEl.style.background = "green";
            buttonEl.innerText = "Stop"
            buttonEl.classList.remove("buttonInactive");
            nextState         = 5;
        }
        else if (engineState == 5) {
            stateTxt = "Stopped";
            stateEl.style.background = "yellow";
            buttonEl.innerText = "Return to Ready";
            buttonEl.classList.remove("buttonInactive");
            nextState         = 2;
        }
        else if (engineState == 6) {
            stateTxt = "ERROR ADC LOW!";
            stateEl.style.background = "red";
            buttonEl.innerText = "Return to Ready";
            buttonEl.classList.remove("buttonInactive");
            nextState         = 2;
        }
        else if (engineState == 7) {
            stateTxt = "ERROR ADC HIGH!";
            stateEl.style.background = "red";
            buttonEl.innerText = "Return to Ready";
            buttonEl.classList.remove("buttonInactive");
            nextState         = 2;
        }
        else if (engineState == 8) {
            stateTxt = "ERROR LinkBad !";
            stateEl.style.background = "red";
            buttonEl.innerText = "reboot device!";
            //buttonEl.classList.remove("buttonInactive");
            nextState         = 0;
        }
        stateEl.innerText = stateTxt;


        lastTimeStamp = dataObj.ts;

        var currentTime = Date.now();
        var delta = currentTime - lastTime;
        lastTime = currentTime;

        array.push(delta);
        if (array.length > 15) array.shift();


        avgDelta = 0;
        array.forEach((e) => { avgDelta += e });
        avgDelta = avgDelta / array.length;

        avgDeltaEl.innerText = "avgDelta: " + (avgDelta).toFixed(0) + "ms";

        readyForData = true;

        //console.log(dataObj);
    }
    else {
        if (this.readyState == 4 && this.status != 200) {
            console.log("status: " + this.status);
            readyForData = true;
        }   
    }
};



xmlhttp.ontimeout = function () {
    console.log("timeout");
    readyForData = true;
}

xmlhttp.onerror = function () {
    console.log("error");
    readyForData = true;
}


function getData(event) {

    var outputPwmValue = -1;
    if (lastInput === pwmSlider) outputPwmValue = pwmSlider.value; 

    if (event | newState) xmlhttp.open("GET", "data/pwm=" + 
    outputPwmValue + 
    ",0," + 
    adcTarget.value + "," + 
    pwmSliderMin.value + "," + 
    pwmSliderMax.value + "," + 
    newState + "," +
    shunt0Slider.value  + "," +
    0,  true);
    else xmlhttp.open("GET", "data", true);
    readyForData = false;
    newState = 0;
    xmlhttp.send();
}

function onclickButton(event) {
    buttonEl.classList.add("buttonInactive");
    console.log("click");
    newState = nextState;
    getData();
}




//var timer = setInterval( getData, 500);
var lastAnimationTime = 0;
var interval = 500;// ms
var requestTime = 0;
function animator(timeStamp) {
    if ((lastAnimationTime + interval < timeStamp) || event) {
        //console.log("" + timeStamp + " " + readyForData);
        lastAnimationTime = Math.floor((timeStamp / interval)) * interval;
        if (readyForData) {

            getData(event);
            event = false;
            requestTime = timeStamp;
        }
    
    }
    if (timeStamp > requestTime + 1000) {
        //console.log("old data!");
        stateEl.innerText = "Link Error " + Math.round( (timeStamp - requestTime) / 1000) + " (secs)";
        stateEl.style.background = "red";
    }
    

    window.requestAnimationFrame(animator);

}


window.requestAnimationFrame(animator)



