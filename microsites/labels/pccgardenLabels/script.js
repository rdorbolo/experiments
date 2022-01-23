'use strict'

var body = document.getElementsByTagName("body")[0];
var labelCount = 0;


function label(line1,line2, line3) {
    var holderDiv = document.createElement("div");
    holderDiv.classList.add("holder");    
    holderDiv.classList.add("holderBackground"+labelCount%4);
    if (labelCount%2 == 0) holderDiv.classList.add("holderLeft");
    else                   holderDiv.classList.add("holderRight");
   
    var innerHolderDiv = document.createElement("div");
    innerHolderDiv.classList.add("innerHolder");

    var line1Div = document.createElement("div");
    var line2Div = document.createElement("div");
    var line3Div = document.createElement("div");

    line1Div.classList.add("line1");
    line2Div.classList.add("line2");
    line3Div.classList.add("line3");

    line1Div.innerHTML = line1;
    line2Div.innerHTML = line2;
    line3Div.innerHTML = line3;

    innerHolderDiv.append(line1Div);
    innerHolderDiv.append(line2Div);
    innerHolderDiv.append(line3Div);

    holderDiv.append(innerHolderDiv);

    //var t=document.createElement("div");
    //t.innerHTML = "test";
    //t.classList.add("innerHolder");
    //holder.append(t);

    body.append(holderDiv);

    labelCount++;


}

body.innerHTML = "";

console.log("hi");


//label(bonnet rose, flower, rosa bonnetus);






/*
label("Mock Orange",	"Philadelphus lewisii",	"Fragrant flowers");
label("Perennial Stock","Matthiola fruticulosa","Fragrant flowers");
label("Sweet Olive",	"Osmanthus delavayi", "Fragrant flowers");
label("Gardenia",	"Gardenia jasminoides 'Frost Proof'",	"Fragrant flowers");
label("False Holly",	"Osmanthus heterophyllus 'Goshiki'",	"Fragrant flowers");
label("Ceanothus",	"Ceanothus 'Dark Star'",	"Fragrant flowers");
label("Butterfly Bush (Sterile)",	"Buddleia 'Asian Moon'",	"Fragrant flowers");
label("Nodding Chocolate Flower",	"Glumicalyx goseloides",	"Fragrant foliage");
label("Bluebeard",	"Caryopteris 'Hint of Gold'",	"Fragrant foliage");
label("Rosemary",	"Rosemarinus officinalis 'Seven Seas'",	"Fragrant foliage");
label("Viburnum",	"Viburnum x bodnantense 'Dawn'",	"Fragrant flowers");
label("Grape Hyacinth",	"Muscari macrocarpum",	"Fragrant flowers");
label("Daisy Bush",	"Olearia macrodonta",	"Fragrant flowers");
label("Oleaster",	"Elaeagnus x ebbingei 'Gilt Edge'",	"Fragrant flowers");
label("Honeysuckle",	"Lonicera 'Harlequin'",	"Fragrant flowers");
label("Rock Rose",	"Cistus laurifolius",	"Fragrant foliage");
label("Willow-leaved Jessamine",	"Cestrum parqui",	"Fragrant flowers in evening");
label("Ocean Spray",	"Holodiscus discolor",	"Fragrant flowers");
*/

label("Sweet Pepperbush",	"Clethra alnifolia 'Ruby Spice'",	"Fragrant flowers");
label("Camellia",	 "Camellia sasanqua 'Setsugekka'",	"Fragrant flowers");
label("Fragrant Abelia",	"Abelia mosanensis",	"Fragrant flowers")
label("Harlequin Glorybower",	"Clerodendrum trichotomum", 	"Fragrant foliage & flowers");
label("Abelia",	"Abelia grandiflora 'Sunshine Daydream'",	"Fragrant flowers");
label("Mexican Orange",	"Choisya ternata 'Aztec Pearl'",	"Fragrant foliage & flowers");
label("Autumn Clematis",	"Clematis terniflora",	"Fragrant flowers in fall");
label("Evergreen Clematis", 	"Clematis armandii",	"Fragrant flowers");
label("Star jasmine",	"Trachelospermum jasminoides",	"Fragrant flowers");
label("Nodding Chocolate Flower",	"Glumicalyx goseloides",	"Fragrant foliage");


