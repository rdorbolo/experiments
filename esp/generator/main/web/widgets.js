

export class slider2 {
    constructor(text, min, max, initialVal, readOnly) {





        const sliderEl = document.createElement('div');
        window[text] = sliderEl;



        sliderEl.oninput = () => console.log("input event unbubbled");

        sliderEl.classList.add("slider2");

        const textAreaEl = document.createElement("div");
        const touchLineEl = document.createElement("div");

        textAreaEl.innerHTML = text;
        touchLineEl.classList.add("touchLine");

        const buttonTouchEl       = document.createElement('div');
        const buttonEchoEl        = document.createElement('div');
        const buttonValueEl       = document.createElement('div');
        const buttonValueMinMaxEl = document.createElement('div');

        buttonTouchEl.classList.add("sliderTarget");
        buttonTouchEl.style.backgroundColor = "rgba(255,255,255,.4)";

        buttonEchoEl.classList.add("sliderTarget");
        buttonEchoEl.style.transition = "all .5s";
        buttonEchoEl.style.backgroundColor = "rgba(0,0,255,.8)";


        buttonValueEl.classList.add("sliderTarget");
        buttonValueEl.style.backgroundColor = "yellow";
        buttonValueEl.style.borderRadius = "100%";

        buttonValueMinMaxEl.classList.add("sliderTarget");
        buttonValueMinMaxEl.style.backgroundColor = "rgba(125,255,255,.4)";
        buttonValueMinMaxEl.style.paddingLeft = "0"
        buttonValueMinMaxEl.style.paddingRight = "0"
        buttonValueMinMaxEl.style.width = "unset";



        if (!readOnly) {
            touchLineEl.appendChild(buttonEchoEl);
            touchLineEl.appendChild(buttonTouchEl);
        }
        touchLineEl.appendChild(buttonValueMinMaxEl);
        touchLineEl.appendChild(buttonValueEl);


        sliderEl.appendChild(textAreaEl);
        sliderEl.appendChild(touchLineEl);

        sliderEl.me = sliderEl;
        touchLineEl.id = text;

        const bodyEl = document.getElementsByTagName("body")[0];

        sliderEl.text                = text;
        sliderEl.textAreaEl          = textAreaEl;
        sliderEl.buttonTouchEl       = buttonTouchEl;
        sliderEl.touchLineEl         = touchLineEl;
        sliderEl.buttonEchoEl        = buttonEchoEl;
        sliderEl.buttonValueEl       = buttonValueEl;
        sliderEl.buttonValueMinMaxEl = buttonValueMinMaxEl

        sliderEl.startx;
        sliderEl.touchX = 0;
        sliderEl.startClientOffset = 0;
        sliderEl.mouseIsDown = false;
        sliderEl.touchIsDown = false;
        sliderEl.valueOut = initialVal;
        sliderEl.percent = (initialVal - min) / (max - min);
        sliderEl.percentTouch = 0;
        sliderEl.percentValue = 0;
        sliderEl.max = max;
        sliderEl.min = min;
        sliderEl.valueMax = 10;
        sliderEl.valueMin = 20;


        Object.defineProperty(this, "valueEcho", {
            get: function () { return this.sliderEl.valueEcho },
            set: function (v) {

                //sliderEl.textAreaEl.innerHTML = sliderEl.text + " " + Math.round(v)
                sliderEl.valueEcho = v;

                //var percentEcho = (v - sliderEl.min)/(sliderEl.max - sliderEl.min);
                sliderEl.percentEcho = (v - sliderEl.min) / (sliderEl.max - sliderEl.min);
                //console.log("echo value is " + v + " percentEcho:" + sliderEl.percentEcho);

                updateValues(sliderEl);
            }
        });

        Object.defineProperty(this, "value", {
            get: function () { window.t = this; return Math.round(this.el.valueOut) },
            set: function (v) {

                //sliderEl.textAreaEl.innerHTML = sliderEl.text + " " + Math.round(v)
                sliderEl.value = v;

                //var percentEcho = (v - sliderEl.min)/(sliderEl.max - sliderEl.min);
                sliderEl.percentValue = (v - sliderEl.min) / (sliderEl.max - sliderEl.min);
                //console.log("the value is " + v + " percentEcho:" + sliderEl.percentValue);
                updateValues(sliderEl);
            }
        });


        //testSlider3.valueMin

        Object.defineProperty(this, "valueMin",{
            set: function (v) {
                sliderEl.valueMin = v;
                sliderEl.buttonValueMinMaxEl.style.left = "" +  100*(v - sliderEl.min) / (sliderEl.max - sliderEl.min) + "%";
            }
        });

        Object.defineProperty(this, "valueMax",{
            set: function (v) {
                sliderEl.valueMax = v;
                sliderEl.buttonValueMinMaxEl.style.right = "" + ( 100  -  100*(v - sliderEl.min) / (sliderEl.max - sliderEl.min) )+ "%";
            }
        });


        sliderEl.updateText = function () {

            if (readOnly) sliderEl.textAreaEl.innerHTML = sliderEl.text + " Value:" + sliderEl.value + " (" + Date.now() + ")";
            else {
                sliderEl.textAreaEl.innerHTML = sliderEl.text + " Value:" + sliderEl.value  + " Out:" + sliderEl.valueOut + " Echo: " + sliderEl.valueEcho +  " (" + Date.now() + ")";
            }
        }
        this.updateText = sliderEl.updateText;

        function updateValues(me /*sliderEl*/) {
            //console.log("this is " + me);
            me.valueOut = Math.round(me.min + me.percent * (me.max - me.min))
            //console.log("me.max: " + me.max +" percent :" + me.percent + " value : " + me.valueOut)

            //me.textAreaEl.innerHTML = me.text + " " + Math.round(me.value);
            me.buttonTouchEl.style.left = "" + 100 * (me.percent      - me.buttonTouchEl.getBoundingClientRect().width / 2 / me.touchLineEl.clientWidth) + "%";
            me.buttonEchoEl.style.left  = "" + 100 * (me.percentEcho  - me.buttonEchoEl.getBoundingClientRect().width / 2 / me.touchLineEl.clientWidth) + "%";
            me.buttonValueEl.style.left = "" + 100 * (me.percentValue - me.buttonValueEl.getBoundingClientRect().width / 2 / me.touchLineEl.clientWidth) + "%";

        }


        sliderEl.ontouchstart = (e) => {
            sliderEl.touchstart = e;
            e.preventDefault();
            console.log("touchstart");
            sliderEl.startx = e.touches[0].screenX;
            var rect = e.target.getBoundingClientRect();
            var offsetX = e.targetTouches[0].pageX - rect.left;
            sliderEl.startClientOffset = offsetX;
            sliderEl.touchIsDown = true;
            sliderEl.touchX = offsetX;
            buttonTouchEl.style.left = "" + sliderEl.touchX + "px";
            console.log("offsetX: " + offsetX);
            console.log("startClientOfffset:   " + sliderEl.startClientOffset);
        }

        sliderEl.onmousedown = (e) => {
            touchLineEl.mousedown = e;
            sliderEl.touchIsDown = true;
            window.activeSlider = sliderEl;
            console.log("mousedown");

            sliderEl.startx = e.screenX;
            sliderEl.startClientOffset = e.offsetX;
            sliderEl.mouseIsDown = true;
            sliderEl.touchX = e.offsetX;

            var slider = window.activeSlider;



            var percent = slider.touchX / slider.touchLineEl.clientWidth;
            sliderEl.percent = percent;
            updateValues(slider);

            sliderEl.dispatchEvent(new InputEvent("input", { bubbles: true, target: sliderEl }))

        };

        bodyEl.ontouchmove = (e) => {
            sliderEl.touchmove = e;


            if (window.activeSlider.touchIsDown) {
                console.log("touchmove");
                //console.log(sliderEl.startClientOffset);
                //console.log(e.touches[0].screenX);
                //console.log(sliderEl.startx);
                sliderEl.touchX = sliderEl.startClientOffset + e.touches[0].screenX - sliderEl.startx;
                buttonTouchEl.style.left = "" + sliderEl.touchX + "px";
                var rect = sliderEl.getBoundingClientRect();
                console.log((buttonTouchEl.getBoundingClientRect().width / 2 + e.touches[0].screenX - rect.left) / rect.width);

            }
        }
        bodyEl.onmousemove = (e) => {
            //sliderEl.mousemove = e;
            var sliderEl = window.activeSlider;


            if (window.activeSlider && window.activeSlider.mouseIsDown) {
                var slider = window.activeSlider;

                slider.touchX = slider.startClientOffset + e.screenX - slider.startx;

                var percent = slider.touchX / slider.touchLineEl.clientWidth;
                //slider.buttonTouchEl.style.left = "" + 100*(percent  - slider.buttonTouchEl.getBoundingClientRect().width / 2 /  slider.touchLineEl.clientWidth ) +  "%";
                slider.percent = percent;

                updateValues(slider);
                sliderEl.dispatchEvent(new InputEvent("input", { bubbles: true, target: sliderEl }))

                //console.log("mousemove: " + "percent: " + percent );
            }
        }

        bodyEl.ontouchend = (e) => {
            sliderEl.touchIsDown = false;
            sliderEl.touchend = e;
            console.log("touchend");

            buttonTouchEl.style.left = "" + sliderEl.touchX + "px";

        }


        bodyEl.onmouseup = (e) => {
            if (window.activeSlider.mouseIsDown) {
                var slider = window.activeSlider;
                slider.mouseIsDown = false;
                sliderEl.mouseup = e;
                console.log("mouseup");
                slider.touchX = slider.startClientOffset + e.screenX - slider.startx - slider.buttonTouchEl.getBoundingClientRect().width / 2;
                //slider.buttonTouchEl.style.left = "" + slider.touchX + "px";
            }

        }



        this.el = sliderEl;
    }
}

