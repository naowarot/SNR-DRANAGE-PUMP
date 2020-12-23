/* Using spreadsheet API */
function doGet(e) { 
  Logger.log( JSON.stringify(e) );  // view parameters
var result = 'Ok'; // assume success
if (e.parameter == undefined) {
    result = 'No Parameters';
  }
  else {
    var id = '1kd_gnUwJtbRd_VUDWBLbOB2-2U6OvsumpmLJfmkCNLo';
    var sheet = SpreadsheetApp.openById(id).getActiveSheet();
    var newRow = sheet.getLastRow() + 1;
    var rowData = [];
    //var waktu = new Date();
    rowData[0] = new Date(); // Timestamp in column A
    
    for (var param in e.parameter) {
      Logger.log('In for loop, param='+param);
      var value = stripQuotes(e.parameter[param]);
      //Logger.log(param + ':' + e.parameter[param]);
      switch (param) {
        case 'value1': //Parameter
          rowData[1] = value; //Value in column B
          break;
        case 'value2':
          rowData[2] = value;
          break;
        case 'value3':
          rowData[3] = value;
          break;
        case 'value4':
          rowData[4] = value;
          break;
        case 'value5':
          rowData[5] = value;
          break;
         case 'value6':
          rowData[6] = value;
          break;
          case 'value7':
          rowData[7] = value;
          break;
          case 'value8':
          rowData[8] = value;
          break;
          case 'value9':
          rowData[9] = value;
          break;
          case 'value10':
          rowData[10] = value;
          break;
          case 'value11':
          rowData[11] = value;
          break;
          case 'value12':
          rowData[12] = value;
          break;
          case 'value13':
          rowData[13] = value;
          break;
          case 'value14':
          rowData[14] = value;
          break;
          case 'value15':
          rowData[15] = value;
          break;
          case 'value16':
          rowData[16] = value;
          break;
          case 'value17':
          rowData[17] = value;
          break;
          case 'value18':
          rowData[18] = value;
          break;
          case 'value19':
          rowData[19] = value;
          break;
          case 'value20':
          rowData[20] = value;
          break;
          case 'value21':
          rowData[21] = value;
          break;
          case 'value22':
          rowData[22] = value;
          break;
          case 'value23':
          rowData[23] = value;
          break;
          case 'value24':
          rowData[24] = value;
          break;
         
         
        default:
          result = "unsupported parameter";
      }
    }
    Logger.log(JSON.stringify(rowData));
// Write new row below
    var newRange = sheet.getRange(newRow, 1, 1, rowData.length);
    newRange.setValues([rowData]);
  }
// Return result of operation
  return ContentService.createTextOutput(result);
}
/**
* Remove leading and trailing single or double quotes
*/
function stripQuotes( value ) {
  return value.replace(/^["']|['"]$/g, "");
}
