$(document).ready(function() {
  console.log('start and get all data');
  $('[data-toggle="tooltip"]').tooltip();
  //getAll();
});

$('#btnRefStat').click(function() {
  console.log('refreshStats');
  // get current stats
  $('#loadModal').modal('show');
  $.get(urlBase + 'settings/status', function(data) {
    $('#loadModal').modal('hide');
    console.log(data);
    $('#chip').text(data.chipid);
    $('#cpu').text(data.cpu + ' MHz');
    $('#heap').text(data.heap + ' Bytes');
    $('#heap').css('width', (data.heap*100)/81920 + '%');
    colorStatusbar($('#heap'));
    $('#flash').text(data.availsize + ' Bytes');
    $('#flash').css('width', (data.availsize*100)/1044464 + '%');
    colorStatusbar($('#flash'));
    $('#spiffs').text(data.availspiffs + ' Bytes');
    $('#spiffs').css('width', (data.availspiffs*100)/data.spiffssize + '%');
    colorStatusbar($('#spiffs'));
    $('#ssidstat').text(data.ssid);
    $('#ip').text(data.ip);
    $('#gate').text(data.gateway);
    $('#mask').text(data.netmask);
    $('#dns').text(data.dns);
    $('#mac').text(data.mac);
  });
});

$('#ssid').change(function() {
  console.log('ssid changed');
  $('#wifiBSSID').val($('#ssid option:selected').val());
});

$('#btnScanBSSID').click(function() {
  console.log('scanWifi');
  // change scan button text to ...
  $('#btnScanBSSID').text('...');
  $('#inputToHide').css('display', 'none');
  $('#ssid').css('display', 'inline');
  // clear network list before get scan for new
  $('#ssid').find('option').remove();
  // get ssids by get request and fill list again
  $('#loadModal').modal('show');
  $.get(urlBase + 'settings/scanwifi', function(data) {
    $('#loadModal').modal('hide');
    if (data.error) {
      $('#btnScanBSSID').trigger('click');
    } else {
      // sort by signal strength
      data.list.sort(function(a,b){return a.rssi <= b.rssi});
      // add all networks to list
      $.each(data.list, function(index, key) {
        $('#ssid').append(
          $('<option></option>')
            .val(key.ssid)
            .attr('bssidvalue',key.bssid)
            .text('BSSID: ' + key.bssid + ', Signal Strength: ' + key.rssi + ', Network: ' + key.ssid)
        );
      });
      $("#ssid").change();
      // change scan button text to rescan
      $('#btnScanBSSID').text('rescan');
    }
  });
});

$('#btnSaveConf').click(function() {
  console.log('saveConf');
  var adminpwd = $('#adminPwd').val();
  if (adminpwd === null || adminpwd === '') {
    alert('Administrator Password cannot be empty');
    return;
  }
  var ssid;
  if ($('#inputToHide').css('display') == 'none') {
    ssid = $('#ssid option:selected').val();
  } else {
    ssid = $('#inputToHide').val();
  }
  var wifimode = 0;
  if ($('#wifiModeOn').hasClass('active')) {
    wifimode = 1;
    $('#wifiBSSID').val(0);
  }
  var datatosend = {
    command: 'configfile',
    bssid: $('#wifiBSSID').val(),
    ssid: ssid,
    wifimode: wifimode,
    wifipwd: $('#wifiPass').val(),
    hostname: $('#hostname').val(),
    adminpwd: adminpwd,
    relay1: { name: $('#relay1Name').val(),
              type: $('#relay1Type option:selected').val(),
              pin: $('#relay1Pin option:selected').val() },
    relay2: { name: $('#relay2Name').val(),
              type: $('#relay2Type option:selected').val(),
              pin: $('#relay2Pin option:selected').val() },
    relay3: { name: $('#relay3Name').val(),
              type: $('#relay3Type option:selected').val(),
              pin: $('#relay3Pin option:selected').val() },
    relay4: { name: $('#relay4Name').val(),
              type: $('#relay4Type option:selected').val(),
              pin: $('#relay4Pin option:selected').val() },
    relay5: { name: $('#relay5Name').val(),
              type: $('#relay5Type option:selected').val(),
              pin: $('#relay5Pin option:selected').val() },
    relay6: { name: $('#relay6Name').val(),
              type: $('#relay6Type option:selected').val(),
              pin: $('#relay6Pin option:selected').val() },
    relay7: { name: $('#relay7Name').val(),
              type: $('#relay7Type option:selected').val(),
              pin: $('#relay7Pin option:selected').val() },
    relay8: { name: $('#relay8Name').val(),
              type: $('#relay8Type option:selected').val(),
              pin: $('#relay8Pin option:selected').val() }
  };
  console.log(datatosend);
  $('#loadModal').modal('show');
  $.jpost(urlBase + 'settings/configfile', datatosend).then(function(data) {
    $('#loadModal').modal('hide');
    console.log(data);
  });
});

$('#btnBackupSet').click(function() {
  console.log('backupSet');
  $('#downloadSet')[0].click();
});

$('#restoreSet').change(function() {
  console.log('restoreSet');
  //get file object
  var file = $('#restoreSet').prop('files')[0];
  if (file) {
    // create reader
    var reader = new FileReader();
    reader.readAsText(file);
    reader.onload = function(e) {
      var json;
      // browser completed reading file - display it
      try {
        json = JSON.parse(e.target.result);
        if (json.command === 'configfile') {
          var x = confirm('File seems to be valid, do you wish to continue?');
          if (x) {
            $('#loadModal').modal('show');
            $.jpost(urlBase + 'settings/configfile', json).then(function(data) {
              $('#loadModal').modal('hide');
              console.log(data);
              alert('Device now should reboot with new settings');
              location.reload();
            });
          }
        }
      } catch (e) {
        alert('Not a valid backup file');
        return;
      }
    };
  }
});

function getAll() {
  $('#loadModal').modal('show');
  $.get(urlBase + 'settings/configfile', function(data) {
    $('#loadModal').modal('hide');
    console.log('listConf', data);
    $('#inputToHide').val(data.ssid);
    $('#wifiPass').val(data.wifipwd);
    $('#adminPwd').val(data.adminpwd);
    $('#hostname').val(data.hostname);

    var relay1Name = (data.relay1.name ? data.relay1.name : 'Relay 1');
    $('#relay1Name').val(relay1Name);
    $('#relay1Type').val(data.relay1.type).change();
    $('#relay1Pin').val(data.relay1.pin).change();
    var relay2Name = (data.relay2.name ? data.relay2.name : 'Relay 2');
    $('#relay2Name').val(relay2Name);
    $('#relay2Type').val(data.relay2.type).change();
    $('#relay2Pin').val(data.relay2.pin).change();
    var relay3Name = (data.relay3.name ? data.relay3.name : 'Relay 3');
    $('#relay3Name').val(relay3Name);
    $('#relay3Type').val(data.relay3.type).change();
    $('#relay3Pin').val(data.relay3.pin).change();
    var relay4Name = (data.relay4.name ? data.relay4.name : 'Relay 4');
    $('#relay4Name').val(relay4Name);
    $('#relay4Type').val(data.relay4.type).change();
    $('#relay4Pin').val(data.relay4.pin).change();
    var relay5Name = (data.relay5.name ? data.relay5.name : 'Relay 5');
    $('#relay5Name').val(relay5Name);
    $('#relay5Type').val(data.relay5.type).change();
    $('#relay5Pin').val(data.relay5.pin).change();
    var relay6Name = (data.relay6.name ? data.relay6.name : 'Relay 6');
    $('#relay6Name').val(relay6Name);
    $('#relay6Type').val(data.relay6.type).change();
    $('#relay6Pin').val(data.relay6.pin).change();
    var relay7Name = (data.relay7.name ? data.relay7.name : 'Relay 7');
    $('#relay7Name').val(relay7Name);
    $('#relay7Type').val(data.relay7.type).change();
    $('#relay7Pin').val(data.relay7.pin).change();
    var relay8Name = (data.relay8.name ? data.relay8.name : 'Relay 8');
    $('#relay8Name').val(relay8Name);
    $('#relay8Type').val(data.relay8.type).change();
    $('#relay8Pin').val(data.relay8.pin).change();

    updateToggleButton('wifiMode', data.wifimode);
    if (data.wifimode === 0) {
      $('#wifiBSSID').val(data.bssid);
      $('#hideBSSID').css('display', 'block');
    }
    var dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(data, null, 2));
    $('#downloadSet').attr({download: data.hostname + '-settings.json',href: dataStr});
  });
}

function handleToggleButton(id, state) {
  if (id.includes('wifiMode')) {
    setWifiMode(id, state);
  } else {
    console.log(id + ' ' + state);
  }
}

function setWifiMode(id, state) {
  console.log(id + ' ' + state);
  if (state === 'ap') {
    $('#hideBSSID').css('display', 'none');
  } else {
    $('#hideBSSID').css('display', 'block');
  }
}

function colorStatusbar(ref) {
  var percentage = ref.css('width').slice(0, -1);
  if (percentage > 50) ref.attr('class','progress-bar progress-bar-success');
  else if (percentage > 25) ref.attr('class','progress-bar progress-bar-warning');
  else ref.attr('class','progress-bar progress-bar-danger');
}
/*
function updateData() {
  console.log("updateData()");
  $.get('DHT', function(data) {
    console.log('updateData(): got Data:', data);
  });
}

updateData();
window.setInterval(updateData, 5000);
*/
