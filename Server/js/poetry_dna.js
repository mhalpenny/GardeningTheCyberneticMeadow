//--SLOW.TECHNOLOGY----------------------------
//Created by Matthew Halpenny

//--VARIABLES----------------------------
//Declares all global variables and arrays that will be used across the sketch

var train, myFont, rs, words, pos, lineNum, intLine, maxLine, lineBool;
var arrayFromPhp = [];
var lineArrayBuffer = [];
var lineArray = [];
var mutationArray = [];
var newMutationArray = [];
var start = false;




//--PRELOAD------------------------------
//Before loading the page this function will execute, this ensures all assets...
//are present before executing any additonal functions

function preload() {
  //recieves JSON data from previous run (formatting, words, mutations, etc.)
  receiveData();
  //load training poems for markov generation
  train = loadStrings('data/poems.txt');
  //font to be displayed for poem
  myFont = loadFont('assets/Barlow-Light.ttf');
}



//--SETUP--------------------------------
//Initial functions and setup before execution

function setup() {

  //creates a canvas variable that allows positioning inside HTML
  var cnv = createCanvas(windowWidth, windowHeight*1);
  //alocate the canavs within the <div> element called "canvas"
  cnv.parent('canvas');
  //draw the background black with transparency (the gif is below embedded in the CSS)
  // background(0, 0, 0, 40);

  //slow framrate to allow JSON fetching to keep up on slow servers
  // frameRate(60);
  //instantiate RiTa lexicon, loads an array of English language words
  //lexicon will be called during mutations for word substitutions
  lexicon = new RiLexicon();
  //create markov object for sentence generation
  //arguement represents n-grades which will change accuracy/randomness
  markovSen = new RiMarkov(4);
  //create markov object for word replacement
  markovWor = new RiMarkov(3);

  //the training text to use in markov chains is loaded in the markov objects here
  markovSen.loadText(train.join(' '));
  markovWor.loadText(train.join(' '));

  //a boolean used to set initial line count, because of the structure of the functions...
  //line count updating (additons, deletions) can interefere with the inital count...
  //this boolean controls when the initial count is used for summation
  lineBool = false;
}

//--DRAW----------------------------------
//Draw will actually only loop through once in this code, the difference between it and...
//setup is similar to preload(), setup loads assets through RiTa fucntions, the reason for...
//this is to limit the mutations to one round per visit, which work more efficiently once the training is properly analyzed.

function draw() {

  //set fill to white
  fill(255);
  //load custom font for use in text command
  textFont(myFont);
  //set text size
  textSize(18);
  //set text alignment
  textAlign(CENTER);
  //text box draw mode
  rectMode(CENTER);

  //timer function ensures the data is loaded before executing the code
  //usually this is done inside preload() but there are some issues using
  //custom functions within preload()in p5.js
  var ms = millis();
  if (ms >= 4000){

  //draw the background black with transparency (the gif is below embedded in the CSS)
  background(0,0,0, 200);

  //analyze text through RiTa functions for mutation()
  analysis();
  console.log("analysis");
  //moderates poem length through random deletion mutations
  spliceLines();
  console.log("spliceLines");
  //display text on screen
  outputText();
  console.log("outputText");
  //call the save function for updating JSON infromation
  saveData();
  console.log("saveData");
  //stop the draw loop
  noLoop();
  console.log("noLoop");
}
}
//--ANALYSIS-----------------------------
// The analysis function reads in string data from arrayfromPhp (see RetrieveData())

function analysis() {

  //initial properties for the loop, lineNum is used to call index values in an array
  //set the index called to the first space, this will be line one
  lineNum = 0;
  //mutationArray stores the post-processed strings that will be outputted and saved
  //each new index must be instantiated as a string
  mutationArray[0] = '';

  //loops through a nested for loop that creates a lineArray that sperates tagged items from...
  //arrayFromPhp into an array where each line is stored as a string
  for (var i = 0; i < arrayFromPhp.length; i++) {

    // each entry in the file has a label "line" - whose value is an array of strings
    lineArray = arrayFromPhp[i].line;
    //measures initial line count in poem, used to control line formatting as the poem mutates
    intLine = lineArray.length;
    //only sum the initial line count once into maxLine
    //maxLine controls the length of mutationArray and what is outputted/saved
    //maxline will be incremented or subtracted from here out depending on the mutation
    if (lineBool == false) {
      maxLine = intLine;
      lineBool = true;
    }

    //this section of the nested for loop will call each line
    for (var j = 0; j < lineArray.length; j++) {

      //creates a temporary string out of the string within lineArrayBuffer
      //the RiString function has difficulties calling array values, they function...
      //better with a singular string variable.
      var sTemp = lineArray[j];
      //if the string is not empty, which usually means a break line...
      //create a RiTa string. These string functions allow processing by further RiTa functions.
      if (sTemp != "") {
        //the RiTa string object which will be used below
        rs = RiString(sTemp);
        //analyze the linguistics of the string
        //the words array will contain each word on that line as an individual string replacement...
        //this is used to modify individual words and modify line lengths without losing data.
        words = rs.words();
        //the pos array does the same as the words array except saves each element as a pos tag...
        //the pos tag indicates how the word functions in the poem, because it line up witht he word array...
        //one can modify words based on pos using the same index number [i], which is done in mutate()
        pos = rs.pos();
      }

        //mutates word array created in analysis() line by line
        mutate();

    }
  }
}

//--MUTATIONS------------------------------
//This function mutates words, sentences, formatting, and pos within the poem.
//mutate() runs in tandem with analysis(). The analysis creates an array of words from each line...
//then mutate processes each line seperately before exiting and being recalled.


function mutate() {

  //loop through each line's words before exiting loop
  for (var i = 0; i < words.length; i++) {
    //creation of probability variables for mutations to occur
    //mutation probability for word substitution
    var mutate = int(random() * 20);
    //mutation probability for formatting
    var lineBr = int(random() * 300);

    //if the word is a noun and the probability logic is satisfied call this mutation.
    //sense pos[] and words[] align in index we can use the same variable.
    if ((pos[i] === 'nn') && (mutate <= 3)) {
      //create a variable for word substitution
      //the word is chosen from a lexicon bank created in setup via RiTa.js
      var missense = lexicon.randomWord('nn');
      //add new word in the current location of the previous word inside mutationArray
      mutationArray[lineNum] += missense;
      //add a space for formatting
      mutationArray[lineNum] += " ";
    }
    //if a mutation does not occur, place the original word into mutationArray
    else {
      //add current word from line into array
      mutationArray[lineNum] += words[i];
      //add a space for formatting
      mutationArray[lineNum] += " ";

    }
    //create another probability variable for random line addition
    //this line will be seperate from the original poem and will be generated via...
    //markov analysis using the training poems supplied
    var newSen = int(random() * 1000);
    //if the line break mutation is met, trigger the following...
    if (lineBr < 3) {
      //increas the line number, meaning shift down a line and create a new string
      lineNum++;
      //increase the total line count in the poem
      maxLine++;
      //instatiate the new line
      mutationArray[lineNum] = '';



      // if triggered, add a new markov generated sentence
    } else if (newSen < 5 || (mutationArray[lineNum] == mutationArray[lineNum - 1])) {
      //increas the line number, meaning shift down a line and create a new string
      lineNum++;
      //increase the total line count in the poem
      maxLine++;
      //instatiate the new line
      mutationArray[lineNum] = '';


      //generate a new markov sentence using a RiTa.js function
      var sen = markovSen.generateSentence();
      //create a new RiTa object for analysis of the sentence
      var riSen = RiString(sen);
      //analyze the generated string to get a word count for approx. length
      var senWords = riSen.words();

      //if the length exceeds 14 words, regenerate the sentence and do this until...
      //the length is below 14 so that it fits in the context of the poem and text box.
      while (senWords.length >= 14) {
        //generate a new markov sentence
        sen = markovSen.generateSentence();
        //reanalyze the sentence while the loop is running
        //the loop will only break when the word count is sufficiently small
        riSen = RiString(sen);
        senWords = riSen.words();
      }

      //add the generated sentence into the mutationArray
      mutationArray[lineNum] += sen;

    }
  }
  // console.log("line " + lineNum + " index " + i + " " + mutationArray[lineNum]);

  //this section marks the break of the for loop inside the mutation function...
  //now increase the line number and prepare for another call, regardless we will need a new line.
  lineNum++;
  //if we are not at the end of the poem, initialize the new line.
  if (lineNum <= maxLine) {
    mutationArray[lineNum] = '';
  }

}

//--SPLICE---------------------------------
//Splice acts as a deletion mutation and will remove whole lines of the poem at a time.
//They only appear when the text gets long enough and are rare but self regulate the poems length.

function spliceLines() {

  ranSwitch = int(random()*10);
  //if the poem is longer than...
  if (mutationArray.length > 16 && ranSwitch < 6) {


    //create variables that will randomly select an index start and splice length from inside the mutationArray.
    var ranIndex, ranLength;

    //the index value is randomly taken from the poems length and will choose a point to start the cut
    ranIndex = int(random(mutationArray.length));
    //the random length will determine how long the cut will be (how many lines)
    ranLength = int(random(2, 10));

    //splice out the determined lines
    //the splice is stored in a temporary array that will be wiped after leaving the function...
    //the remaining mutationArray is modified in length and will have those elemtns removed
    var tempArray = mutationArray.splice(ranIndex, ranLength);


  } else if (mutationArray.length > 10 && ranSwitch > 6){


        //create variables that will randomly select an index start and splice length from inside the mutationArray.
        var ranIndex, ranLength;

        //the index value is randomly taken from the poems length and will choose a point to start the cut
        // ranIndex = int(random(mutationArray.length));
        ranIndex = int(random(2, 5));
        //the random length will determine how long the cut will be (how many lines)
        ranLength = int(random(2, 5));

        //splice out the determined lines
        //the splice is stored in a temporary array that will be wiped after leaving the function...
        //the remaining mutationArray is modified in length and will have those elemtns removed
        var tempArray = mutationArray.splice((maxLine - ranIndex), ranLength);

  }
}

//--OUTPUT---------------------------------
//Output recieves the reconstructed string arrays from mutation via mutationArray...
//and constructs the outputted text to be displayed on screen then after displaying...
//the text calls the save function

function outputText() {

  //create an offset variable for displaying lines horizontally
  var offsetY = 150;

  //loop through mutationArray at the designated poem lenght (maxLine)
  for (var i = 0; i < maxLine; i++) {

    //draw text from mutationArray at the given coordinates
    text(mutationArray[i], width / 2, (height / 2) + offsetY, windowWidth, windowHeight);

    //increase the offset for each line by 20px
    offsetY += 20;

  }

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
      'stringData': mutationArray
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
  var fileName = 'txt/lineFormatNewCopy.json';
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

function mousePressed() {
   clear();
   console.log('clear');
   redraw(1);
 }
