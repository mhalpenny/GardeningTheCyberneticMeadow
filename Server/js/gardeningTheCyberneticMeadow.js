window.onload = function() {
  var rm, txt0, txt1, txt2, txt3, txt4, txt5, txt6, referenceText, novella, udpWake, json;
  var power = false;
  var wordCount = 0;
  var sentenceLength = 0;
  var results = [];
  results[0] = '';
  // Check current text generation status
  receiveData();
  wordCount = parseInt(json[10]);

  if (wordCount <= 7 && wordCount != 0){
    saveWord(wordCount);
    wordCount++;
    json[10] = wordCount;
    saveSentence();
  } else {
  //generate random dataset selection
  var randomSelection1 = Math.random(6);
  var randomSelection2 = Math.random(6);
  var randomSelection3 = Math.random(6);
  var randomSelection4 = Math.random(6);
  // Load text datasets
  txt0 = loadStrings('data/geology.txt');
  txt1 = loadStrings('data/mix.txt');
  txt2 = loadStrings('data/slow.txt');
  txt3 = loadStrings('data/moss.txt');
  txt4 = loadStrings('data/taffel2.txt');
  txt5 = loadStrings('data/makers4.txt');
  txt6 = loadStrings('data/loveless.txt');
  // Set Markov value
  rm = RiTa.markov(2);
  // Use random dataset selection
  rm.addText(parseInt("txt" + randomSelection1));
  rm.addText(parseInt("txt" + randomSelection2));
  rm.addText(parseInt("txt" + randomSelection3));
  rm.addText(parseInt("txt" + randomSelection4));

  results = rm.generate(10);
  // console.log("size: " + rm.size);

  json = results.toJSON();
  //console.log(json);
  saveSentence();
  }

  if (power == true){
    wordCount = 0;
  }
}

//--SAVE-----------------------------------
//The save function sends the entire results to the JSON file, the PHP function is stored seperately in saveToFile.php
function saveSentence() {
  console.log("send data");
  $.ajax({
    type: "POST",
    url: "saveToFile.php",
    data: {
      'stringData': json
    },
    success: function(resultData) {
      console.log("success");
    }
  });
}

//This save function sends only the latest word
function saveWord(int) {
  console.log("send data");
  $.ajax({
    type: "POST",
    url: "saveToFileWord.php",
    data: {
      'stringData': json[int]
    },
    success: function(resultData) {
      console.log("success");
    }
  });
}

//--LOAD-----------------------------
//The JSON file is saved within the sketch folder, its data is pushed to arrayFromPhp
function receiveData() {
  console.log("data incoming");
  var fileName = 'currentText.json';
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
      sentenceLength = data.length;
      wordCount++;
      console.log("done");
    }
  });
  start = true;
}