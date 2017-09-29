$(document).ready(function() {
  $('[data-toggle="tooltip"]').tooltip();
});

$('#btnRefStat').click(function() {
  // get current stats
  $('#loadModal').modal('show');
  $.get(urlBase + 'settings/status', function(data) {
    $('#loadModal').modal('hide');
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
  $('#btnRefStat').css('display', 'none');
  $('#btnRefStat2').css('display', 'block');
});

$('#btnRefStat2').click(function() {
  $('#btnRefStat2').css('display', 'none');
  $('#btnRefStat').css('display', 'block');
});

$('#btnSaveConf').click(function() {
  var adminpwd = $('#adminPwd').val();
  if (adminpwd === null || adminpwd === '') {
    alert('Administrator Password cannot be empty');
    return;
  }
  var datatosend = {
    command: 'configfile',
    hostname: $('#hostname').val(),
    adminpwd: adminpwd,
    ssid: $('#ssid').val(),
    wifipwd: $('#wifipwd').val(),
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
  $.jpost(urlBase + 'settings/configfile', datatosend);
  updateSettingsDialog();
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
            $.jpost(urlBase + 'settings/configfile', json);
            updateSettingsDialog();
          }
        }
      } catch (e) {
        alert('Not a valid backup file');
        return;
      }
    };
  }
});

function updateSettingsDialog() {
  $('#updateModal').modal('show');
  setTimeout(function(){
    location.reload();
    $('#updateModal').modal('hide');
  }, 10000);
}

function getAll() {
  $('#loadModal').modal('show');
  $.get(urlBase + 'settings/configfile', function(data) {
    $('#loadModal').modal('hide');
    $('#adminPwd').val(data.adminpwd);
    $('#hostname').val(data.hostname);
    $('#ssid').val(data.ssid);
    $('#wifipwd').val(data.wifipwd);
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
    var dataStr = 'data:text/json;charset=utf-8,' + encodeURIComponent(JSON.stringify(data, null, 2));
    $('#downloadSet').attr({download: data.hostname + '-settings.json',href: dataStr});
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
}

function colorStatusbar(ref) {
  var width = ref.width();
  if (!ref.css('width').includes('%')) {
    var parentWidth = ref.offsetParent().width();
    var percent = 100*width/parentWidth;
  } else {
    var percent = width;
  }
  if (percent > 50) ref.attr('class','progress-bar progress-bar-success');
  else if (percent > 25) ref.attr('class','progress-bar progress-bar-warning');
  else ref.attr('class','progress-bar progress-bar-danger');
}
