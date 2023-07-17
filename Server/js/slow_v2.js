//--INFO----------------------------
//Created by Matthew Halpenny
//Based of Poetry.DNA

//--VARIABLES----------------------------
//Declares all global variables and arrays that will be used across the sketch

var train1, train2, train3, train4, train5myFont, rs, words, pos, lineNum, intLine, maxLine, lineBool, rm;
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
    //font to be displayed for poem
    myFont = loadFont('assets/Barlow-Light.ttf');
    
  }

  function setup() {

    var cnv = createCanvas(windowWidth, windowHeight*1);
    cnv.parent('canvas');
    // createCanvas(windowWidth, windowHeight*0.8);
    background(255, 255, 255, 40);
    textSize(20);
    noStroke();
    fill(0);
    textFont(myFont);
    textSize(12);
    textAlign(CENTER);
    rectMode(CORNER);
    frameRate(60);

    train = loadStrings('text/slow.txt');
    train2 = loadStrings('text/jussi.txt');
    train3 = loadStrings('text/geology.txt');
    train4 = loadStrings('text/violence.txt');
    train5 = loadStrings('text/violence2.txt');

    // var ms = millis();
    // if (ms >= 20000){

      //a boolean used to set initial line count, because of the structure of the functions...
      //line count updating (additons, deletions) can interefere with the inital count...
      //this boolean controls when the initial count is used for summation
      lineBool = false;
    }
  // }

  function draw() {
    var ms = millis();
    if (ms >= 40000){
    if (loopSW == true){
            //create markov object for word replacement
            rm = RiTa.markov(3);

            //the training text to use in markov chains is loaded in the markov objects here
            rm.addText(train1);
            rm.addText(train2);
            rm.addText(train3);
            rm.addText(train4);
            rm.addText(train5);

        // background(255,255,255, 200);
        generateText();
        
        if (generated == true){
            outputText();
        }
    }
  }
  }

//--GENERATE-------------------------------


function generateText(){

    var wordNum = 0;
    results[0] = '';
    
    // for (var i = 0; i < 4; i++) {
    // //generate a new markov sentence using a RiTa.js function
    //  var newWord = rm.generateSentence();
    //  results[wordNum] = newWord;
    //  console.log("generated: " + wordNum);
    //  console.log(results[i]);
    //  wordNum++;
    // }

    // for (var i = 0; i < 10; i++) { 
    //   results[i] = rm.generate(1);
    // }

    results = rm.generate(30);

    generated = true;


}
//--OUTPUT---------------------------------
//Output recieves the reconstructed string arrays from mutation via mutationArray...
//and constructs the outputted text to be displayed on screen then after displaying...
//the text calls the save function

function outputText() {

    //create an offset variable for displaying lines horizontally
    var offsetY = 200;
    var offsetX = 0;
  
    //loop through mutationArray at the designated poem length
    for (var i = 0; i < results.length; i++) {
  
      //draw text from mutationArray at the given coordinates
      text(results[i], 0, 0 + offsetY, windowWidth, windowHeight);
        
    //   if (offsetX >= 120){
    //       offsetX = 0;
    //       offsetY += 20;
    //   }

      //increase the offset for each line by 20px
      offsetY += 10;
        
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
  