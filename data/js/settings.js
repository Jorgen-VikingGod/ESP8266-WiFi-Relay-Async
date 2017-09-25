$(document).ready(function() {
  $('[data-toggle="tooltip"]').tooltip();
});

function onOpen(evt) {
  websock.send('{"command":"getconf"}');
}

function onMessage(evt) {
  websock.onmessage = function(evt) {
    var json = JSON.parse(evt.data);
    if (json.command === 'ssidlist') {
      listSSID(json);
    } else if (json.command === 'configfile') {
      listCONF(json);
    } else if (json.command === 'status') {
      listStats(json);
    } else if (json.command === 'toggle') {
      getToggle(json);
    }
  };
}

$('#btnRefStat').click(function() {
  websock.send('{"command":"status"}');
  $('#loading').css('display', 'block');
});

$('#ssid').change(function() {
  $('#wifiBSSID').val($('#ssid option:selected').attr('bssidvalue'));
});

$('#btnScanBSSID').click(function() {
  websock.send('{"command":"scan"}');
  // change scan button text to ...
  $('#btnScanBSSID').text('...');
  $('#inputToHide').css('display', 'none');
  $('#ssid').css('display', 'inline');
  // clear network list before get scan for new
  $('#ssid').find('option').remove();
  // get ssids by get request and fill list again
  $('#loading').css('display', 'block');
});

$('#btnSaveConf').click(function() {
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
    relay1: { type: $('#relay1Type option:selected').val(),
              pin: $('#relay1Pin option:selected').val() },
    relay2: { type: $('#relay2Type option:selected').val(),
              pin: $('#relay2Pin option:selected').val() },
    relay3: { type: $('#relay3Type option:selected').val(),
              pin: $('#relay3Pin option:selected').val() },
    relay4: { type: $('#relay4Type option:selected').val(),
              pin: $('#relay4Pin option:selected').val() },
    relay5: { type: $('#relay5Type option:selected').val(),
              pin: $('#relay5Pin option:selected').val() },
    mask1:  { open: $('#mask1Open option:selected').val(),
              close: $('#mask1Close option:selected').val(),
              pin: $('#mask1Pin option:selected').val() },
    mask2:  { open: $('#mask2Open option:selected').val(),
              close: $('#mask2Close option:selected').val(),
              pin: $('#mask2Pin option:selected').val() },
    mask3:  { open: $('#mask3Open option:selected').val(),
              close: $('#mask3Close option:selected').val(),
              pin: $('#mask3Pin option:selected').val() },
  };
  $('#loading').css('display', 'block');
  websock.send(JSON.stringify(datatosend));
  location.reload();
});

$('#btnBackupSet').click(function() {
  $('#downloadSet')[0].click();
});

$('#restoreSet').change(function() {
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
            $('#loading').css('display', 'block');
            websock.send(JSON.stringify(json));
            alert('Device now should reboot with new settings');
            location.reload();
          }
        }
      } catch (e) {
        alert('Not a valid backup file');
        return;
      }
    };
  }
});

function listCONF(data) {
  $('#loading').css('display', 'none');
  $('#inputToHide').val(data.ssid);
  $('#wifiPass').val(data.wifipwd);
  $('#adminPwd').val(data.adminpwd);
  $('#hostname').val(data.hostname);
  $('#relay1Type').val(data.relay1.type).change();
  $('#relay1Pin').val(data.relay1.pin).change();
  $('#relay2Type').val(data.relay2.type).change();
  $('#relay2Pin').val(data.relay2.pin).change();
  $('#relay3Type').val(data.relay3.type).change();
  $('#relay3Pin').val(data.relay3.pin).change();
  $('#relay4Type').val(data.relay4.type).change();
  $('#relay4Pin').val(data.relay4.pin).change();
  $('#relay5Type').val(data.relay5.type).change();
  $('#relay5Pin').val(data.relay5.pin).change();
  $('#mask1Open').val(data.mask1.open).change();
  $('#mask1Close').val(data.mask1.close).change();
  $('#mask1Pin').val(data.mask1.pin).change();
  $('#mask2Open').val(data.mask2.open).change();
  $('#mask2Close').val(data.mask2.close).change();
  $('#mask2Pin').val(data.mask2.pin).change();
  $('#mask3Open').val(data.mask3.open).change();
  $('#mask3Close').val(data.mask3.close).change();
  $('#mask3Pin').val(data.mask3.pin).change();
  updateToggleButton('wifiMode', data.wifimode);
  if (data.wifimode === 0) {
    $('#hideBSSID').css('display', 'none');
    $('#hideSSID').css('display', 'none');
  }
  var dataStr = 'data:text/json;charset=utf-8,' + encodeURIComponent(JSON.stringify(data, null, 2));
  $('#downloadSet').attr({download: data.hostname + '-settings.json',href: dataStr});
}

function listSSID(data) {
  $('#loading').css('display', 'none');
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
    $('#ssid').change();
    // change scan button text to rescan
    $('#btnScanBSSID').text('rescan');
  }
}


function listStats(data) {
  $('#loading').css('display', 'none');
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
}

function handleToggleButton(id, state) {
  if (id.includes('wifiMode')) {
    setWifiMode(id, state);
  } else {
    console.log(id + ' ' + state);
  }
}

function setWifiMode(id, state) {
  if (state === 'ap') {
    $('#hideBSSID').css('display', 'none');
    $('#hideSSID').css('display', 'none');
  } else {
    $('#hideBSSID').css('display', 'block');
    $('#hideSSID').css('display', 'block');
  }
}

function colorStatusbar(ref) {
  var percentage = ref.css('width').slice(0, -1);
  if (percentage > 50) ref.attr('class','progress-bar progress-bar-success');
  else if (percentage > 25) ref.attr('class','progress-bar progress-bar-warning');
  else ref.attr('class','progress-bar progress-bar-danger');
}
