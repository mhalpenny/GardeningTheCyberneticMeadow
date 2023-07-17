//--INFO----------------------------
//Created by Matthew Halpenny
//Based of Poetry.DNA

//--VARIABLES----------------------------
//Declares all global variables and arrays that will be used across the sketch

var train, myFont, rs, words, pos, lineNum, intLine, maxLine, lineBool, rm;
var arrayFromPhp = [];
var proseArray = [];
var newProseArray = [];
var results = [];
var loopSW = true;
var processing, generated = false;

//--PRELOAD------------------------------
//Before loading the page this function will execute, this ensures all assets...
//are present before executing any additonal functions

function preload() {
    //recieves JSON data from previous run (formatting, words, mutations, etc.)
    receiveData();
    //load training poems for markov generation
    train = loadStrings('text/slow.txt');
    //font to be displayed for poem
    myFont = loadFont('assets/Barlow-Light.ttf');
  }

  function setup() {

    //creates a canvas variable that allows positioning inside HTML
    var cnv = createCanvas(windowWidth, windowHeight*1);
    //alocate the canavs within the <div> element called "canvas"
    cnv.parent('canvas');
    //draw the background black with transparency (the gif is below embedded in the CSS)
    // background(0, 0, 0, 40);
  
    //slow framrate to allow JSON fetching to keep up on slow servers
    frameRate(60);

    //set fill to white
    fill(0);
    //load custom font for use in text command
    textFont(myFont);
    //set text size
    textSize(18);
    //set text alignment
    textAlign(CENTER);
    //text box draw mode
    rectMode(CENTER);

    //create markov object for word replacement
    rm = new RiMarkov(3);

    //the training text to use in markov chains is loaded in the markov objects here
    rm.loadText(train.join(' '));

    //a boolean used to set initial line count, because of the structure of the functions...
    //line count updating (additons, deletions) can interefere with the inital count...
    //this boolean controls when the initial count is used for summation
    lineBool = false;
  }

  function draw() {

    if (loopSW == true){
        background(255,255,255, 200);
        generateText();
        
        if (generated == true){
            outputText();
        }
    }
  }

//--GENERATE-------------------------------


function generateText(){

    var wordNum = 0;
    results[0] = '';
    
    for (var i = 0; i < 4; i++) {
    //generate a new markov sentence using a RiTa.js function
     var newWord = rm.generateSentence();
     results[wordNum] = newWord;
     console.log("generated: " + wordNum);
     console.log(results[i]);
     wordNum++;
    }

    generated = true;


}
//--OUTPUT---------------------------------
//Output recieves the reconstructed string arrays from mutation via mutationArray...
//and constructs the outputted text to be displayed on screen then after displaying...
//the text calls the save function

function outputText() {

    //create an offset variable for displaying lines horizontally
    var offsetY = 150;
    var offsetX = 0;
  
    //loop through mutationArray at the designated poem length
    for (var i = 0; i < results.length; i++) {
  
      //draw text from mutationArray at the given coordinates
      text(results[i], (width/2) , (height / 2) + offsetY, windowWidth, windowHeight);
        
    //   if (offsetX >= 120){
    //       offsetX = 0;
    //       offsetY += 20;
    //   }

      //increase the offset for each line by 20px
      offsetY += 20;
        
      console.log("printed line:  " + i);
      console.log(results[i]);
  
    }

    console.log("printing done");
    loopSW = false;
    saveData();
  }

  //--SAVE-----------------------------------
//The save function sends the entire mutationArray to the JSON file...
//the PHP function is stored seperately in saveToFile.php

function saveData() {
    console.log("send data");
    $.ajax({
      type: "POST",
      url: "saveToFile.php",
      data: {
        'stringData': results
      },
      success: function(resultData) {
        console.log("success");
      }
    });
  }
  

  //--LOAD-----------------------------
//The load function receives JSON data from the previous mutated poem using ajax...
//The JSON file is saved within the sketch folder, its data is pushed to arrayFromPhp

function receiveData() {
    console.log("data incoming");
    var fileName = 'current.json';
    $.ajax({
      //don't cache inforamtion so each load is up to date
      cache: false,
      url: fileName,
      success: function(data) {
        // do something now that the data is loaded
        $.each(data, function(index, value) {
          //debug
          console.log(value);
          // put each value into an array
          arrayFromPhp.push(value);
        });
        console.log("done");
      }
    });
    start = true;
  }


//--RESIZE-----------------------------
//If the window is resized, adjust the canvas position *may affect formatting*

function windowResized() {
    resizeCanvas(windowWidth, windowHeight*0.8);
  }
  