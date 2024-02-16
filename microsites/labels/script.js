'use strict'

console.log("hello world");

var TAGS_REQD   = 1;
var GROUP       = 2;
var COMMON_NAME = 3;
var VARIETY     = 4;
var LATIN_NAME  = 5;
var EXPOSURE    = 6;
var HEIGHT      = 7;
var WIDTH       = 8;
var WATER       = 9;
var BLOOM_INFO  = 10;
var UNDERLINE   = 24;

var body = document.getElementsByTagName("body")[0];

//var urlBase = 'http://docs.google.com/feeds/download/spreadsheets/Export';
//var file_id ="1nFHWrJCpSuChvmC85ys8Ov92j2dvZqQVW9-k-QqBMxc";
//var spreadsheetAction = "&exportFormat=tsv&gid=0";

//var url_no_proxy = urlBase + "?key=" + file_id + spreadsheetAction;

//var url = "../servlet/proxy";


//url = 'http://docs.google.com/spreadsheets/d/1nFHWrJCpSuChvmC85ys8Ov92j2dvZqQVW9-k-QqBMxc/export?format=tsv&id=1nFHWrJCpSuChvmC85ys8Ov92j2dvZqQVW9-k-QqBMxc&gid=0';
//var url = 'https://docs.google.com/feeds/download/spreadsheets/Export?key=1fre47EXfic5xV4M2qjTpoRGKED9XCImZ14_F8XZFnIA&exportFormat=tsv&gid=0'

const defaultDocId = '1nFHWrJCpSuChvmC85ys8Ov92j2dvZqQVW9-k-QqBMxc';
var docId;// = '1nFHWrJCpSuChvmC85ys8Ov92j2dvZqQVW9-k-QqBMxc'
let sheetname = "2024"
var b;
var spreadsheet;
var labels  = 0;

var statsDiv;
var labelCountDiv;
var docIdHolderDiv;
let docSheetnameDiv;
var docIdDiv;
var docIdEditDiv;
var docTitleDiv;

var debug = "rick";

docId = localStorage.getItem("docId");
if (docId == null) docId = defaultDocId;

statsDiv = document.createElement("div");
statsDiv.classList.add("statsDiv");
body.append(statsDiv);

labelCountDiv = document.createElement("div");
labelCountDiv.classList.add("labelCountDiv");
labelCountDiv.innerHTML = "Nothing Loaded";

docTitleDiv = document.createElement("div");
docTitleDiv.classList.add("docTitleDiv");
docTitleDiv.innerHTML = "Spreadsheet not loaded";

docIdHolderDiv = document.createElement("div");
docIdHolderDiv.classList.add("docIdHolderDiv");

docIdDiv = document.createElement("div");
docIdDiv.classList.add("docIdDiv");
docIdDiv.innerHTML = "doc id: " + docId;

docSheetnameDiv = document.createElement("div");
docSheetnameDiv.innerHTML = 'sheetname: "' + sheetname + '" (hardcoded in Js)'; 

docIdEditDiv = document.createElement("div");
docIdEditDiv.classList.add("docIdEditDiv");
docIdEditDiv.innerHTML = "&#128393";

docIdEditDiv.onclick = (e) =>{
	console.log(e);
	docId = prompt("Please enter spreedsheet doc id:", defaultDocId);
  if (docId == null || docId == "") {
    docId = defaultDocId;
  } 
  localStorage.setItem("docId", docId);
  docIdDiv.innerHTML = "doc id: " + docId;
  //loadDoc();
  loadMeta();
}


docIdHolderDiv.append(docIdDiv);
docIdHolderDiv.append(docIdEditDiv);

statsDiv.append(docTitleDiv);
statsDiv.append(labelCountDiv);
statsDiv.append(docIdHolderDiv);
statsDiv.append(docSheetnameDiv);



function sget(row, index, theClass) {
	var text = row[index].trim();
	if (text.length == 0) return null;
	var div = document.createElement("xxx");
	div.innerText = text;
	div.classList.add(theClass);
	div.classList.add("info");	
	return div;
}
function sget2(row, index, theClass) {
	var cell = row[index];
	var text;
	if (cell) text = row[index].trim();
	else      text = "";
	if (text.length == 0) return null;

	return text;
}



function createDoc() {

	body.innerHTML = "";
	body.append(statsDiv);
	labels =0;

	    for (var i = 0; i< spreadsheet.length; i++) {
    	var row = spreadsheet[i]
    	var tags_reqd = parseInt(row[TAGS_REQD]);
    	if (tags_reqd < 0) tags_reqd = 0; // remove NaN condition - not needed

    	for (var j =0; j<tags_reqd; j++) {

			labels++;
    		//if (j<4) console.log(row);
			var group      = sget(row, GROUP, "group");
			var commonName = sget2(row, COMMON_NAME, "commonName");
			var variety    = sget2(row, VARIETY, "variety");
			var latinName  = sget2(row, LATIN_NAME, "latinName");
			var exposure   = sget2(row, EXPOSURE, "exposure");
			var height     = sget2(row, HEIGHT, "height");
			var width      = sget2(row, WIDTH, "width");
			var water      = sget2(row, WATER, "water");
			var bloomInfo  = sget2(row, BLOOM_INFO, "bloomInfo");
			var underline  = sget2(row, UNDERLINE, "underline");

			if (underline) console.log(underline);
			// modify the commonName if one of the words is the underline word
			var commonArray = commonName.split(' ');
			console.log(commonName);
			console.log(commonArray);
			if (underline) {
				commonName = "";
				commonArray.forEach(element => {
					debug = element;
					console.log("currentElement=" + element);
					if (element.toLowerCase() == underline.toLowerCase()) element = '<span style="text-decoration: underline">' + element + "</span>"
					commonName += element + " ";
				});
			}

	


			var div = document.createElement("DIV");
			div.classList.add("label");

			var image  =document.createElement("IMG");
			//image.setAttribute("src", "osumg.png");
			image.setAttribute("src", "osu.jpg");


			var span = document.createElement("SPAN");
			span.classList.add("holder");
			

			var description = document.createElement("DIV");
			description.classList.add("description");
			description.appendChild(span);


			var innerHTML = ""; 
			if (commonName) innerHTML += '<span class="commonName">' + commonName + "" + '</span>';
			if (variety)    innerHTML += ' \'' + variety + '\'';
			if (latinName)  innerHTML += '<em>' + " "  + latinName + "" + '</em>';
			innerHTML  += ", ";
			if (height)     innerHTML += " " + height + "H,";
			if (width)      innerHTML += " " + width + "W,";
			if (exposure)   innerHTML += " " + exposure + ",";
			if (water)      innerHTML += " " + water + ",";
			if (bloomInfo)  innerHTML += " " + bloomInfo;

			span.innerHTML = innerHTML;

			var fixed = document.createElement("DIV");
			fixed.classList.add("fixed");

			var message = document.createElement("DIV");
			message.innerHTML = "Washington County Master Gardener&#8482 Association: washingtoncountymastergardeners.org";
			message.classList.add("message");

			if (group) fixed.appendChild(group);
			fixed.appendChild(message);

			div.appendChild(image);
			div.appendChild(description);
			div.appendChild(fixed);
			body.appendChild(div);
    	}
    }

	//document.getElementById("directory").classList.add("hidden");
	
	labelCountDiv.innerHTML = "Total Labels: " + labels;
}



var test;

function loadDoc() {
	labelCountDiv.innerHTML = "Loading data values"
	//fetch('https://sheets.googleapis.com/v4/spreadsheets/' + docId + '/values/Collected?key=AIzaSyCcxs61NVz6a21Q-v3bDC1nHF3Ctt3aBP4')
	fetch('https://sheets.googleapis.com/v4/spreadsheets/' + docId + '/values/' + sheetname + '?key=AIzaSyCcxs61NVz6a21Q-v3bDC1nHF3Ctt3aBP4')
		.then(function (response) {
			console.log("response.type (normally cors): " + response.type);
			response.json().then(
				(r) => {
					test = r;
					//console.log(r)
					spreadsheet = r.values;
					createDoc();
				}
			);
		}
	);
}

//loadDoc();

var test2;
function loadMeta() {

docTitleDiv.innerHTML =  "Loading title ...";
labelCountDiv.innerHTML = "Loading Labels";
fetch('https://sheets.googleapis.com/v4/spreadsheets/' + docId + '?key=AIzaSyCcxs61NVz6a21Q-v3bDC1nHF3Ctt3aBP4')
.then( (response) => {
	console.log(response); 
	test2 = response; 

	if (response.ok) return response.json()
	else throw response.status; 
	}
	)
//.catch(error => console.log('Error1:', error))
.then(
		(r) => {
			console.log("Spreadsheet Meta Info");
			console.log(r);
			test = r;
			docTitleDiv.innerHTML =  r.properties.title;
			loadDoc();
		}
	).catch(error => {	
		console.log('Error2:', error);
		docTitleDiv.innerHTML = "Error loading doc. Error " + error;
	}
	);
}


loadMeta();
