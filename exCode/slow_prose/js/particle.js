particle.login({username: 'email@example.com', password: 'pass'}).then(
    function(data){
      console.log('API call completed on promise resolve: ', data.body.access_token);
    },
    function(err) {
      console.log('API call completed on promise fail: ', err);
    }
  );

  var Particle = require('particle-api-js');
var particle = new Particle();
var token;

particle.login({username: 'user@email.com', password: 'pass'}).then(
  function(data) {
    token = data.body.access_token;
  },
  function (err) {
    console.log('Could not log in.', err);
  }
);

var token; // from result of particle.login
var devicesPr = particle.listDevices({ auth: token });

devicesPr.then(
  function(devices){
    console.log('Devices: ', devices);
  },
  function(err) {
    console.log('List devices call failed: ', err);
  }
);

var fnPr = particle.callFunction({ deviceId: 'DEVICE_ID', name: 'brew', argument: 'D0:HIGH', auth: token });

fnPr.then(
  function(data) {
    console.log('Function called succesfully:', data);
  }, function(err) {
    console.log('An error occurred:', err);
  });

  particle.getVariable({ deviceId: 'DEVICE_ID', name: 'temp', auth: token }).then(function(data) {
    console.log('Device variable retrieved successfully:', data);
  }, function(err) {
    console.log('An error occurred while getting attrs:', err);
  });