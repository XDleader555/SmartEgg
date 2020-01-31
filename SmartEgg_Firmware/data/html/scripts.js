// Session storage is used to store name - data values

//List all drops history
let m_DropDataList = [];

// Keep track of data currently being loaded
let m_loadingList = [];

//Index of the active tab
let m_activeDropIndex; 

// For production with egg
let urlEndPoint = "/functions";

// Tracks the next unique identifier for the drops
let m_nextDropId = 0;
//TODO pull tab by the id not the index

// Document setup after everything has been loaded
$(document).ready(function() {
	//setup
	// Hide the stop button on load
	$("#stop-div").hide();
  $("#download-div").hide();
  $("#loading-div").hide();
  $("#starting-div").hide();
  $("#modal-delete-btn-loading").hide();
    $("#my-modal").hide();

  // $("#main-content").hide();
	updateDropList();
  isConnected();
  getVersion();
  getBattLevel();
  getAPName();
});

// Start the recording action and toggle the appropriate areas
function startRecord() {
  // Toggle start and stop button
  $("#record").hide();
  $("#starting-div").show();

  //Call ajax start record
  ajaxStartRecord();
  
}

// Asynchronous call to start recording
function ajaxStartRecord(){
  let curDropName = m_DropDataList[m_activeDropIndex].name;

  let requestStr = urlEndPoint+"/recordStart/" + curDropName;
  return $.ajax({
    dataType: 'text',
    url: requestStr
  }).done(function(response) {
    // If successful
      console.log(response);

      // Hide the starting button
      $("#starting-div").hide();

      if(response != 0 )
      {
        //bad record force start and alert
        alert("Error with egg record, forcing stop. Try starting the drop again.");

        // Toggle start and stop button so the user may try again
        $("#record").show();
        $("#stop-div").hide();

        // timeout error stops recording
        if(response == -2 )
        {
          HTTPRequest(urlEndPoint+"/recordStop", function(response) {
            console.log("recordStop Status: " + response);
          });
        }

      }
      else
      {
        // Record started
        HTTPRequest(urlEndPoint+"/disableWifi", function(response) {
          console.log("wifi disabled: " + response);
        });

        // show stop button
        $("#stop-div").show();
      }
  }).fail(function(jqXHR, textStatus, errorThrown) {
    // If start fail
    console.log(textStatus + ': ' + errorThrown);
  })
}

// Stop the data then plot it on the graph
function stopRecord() {
  //Disable and show loading
  $("#stop-div").hide();
  $("#loading-div").show();

  // Stop the recording then get the drop data
  ajaxStopRecord().then(ajaxGetDropData);
  
}

function ajaxStopRecord(){
  return $.ajax({
    dataType: 'text',
    url: urlEndPoint+'/recordStop'
  }).done(function(data) {
    // If successful
    console.log("stopping status: " + data);
  }).fail(function(jqXHR, textStatus, errorThrown) {
    // If fail
    console.log(textStatus + ': ' + errorThrown);
  });
}

// Ajax request to get and plot the data
function ajaxGetDropData() {
  let curDrop = m_DropDataList[m_activeDropIndex]
  let getDataUrl = urlEndPoint+"/recordGetMagnitudes/" + curDrop.name;

  return $.ajax({
    dataType: 'text',
    url: getDataUrl
  }).done(function(data) {
    // If successful
    console.log("data retrieved: " + data);

    //Plot data
    let magnitudeData= parseDataMagnitude(data);
    curDrop.chartData = magnitudeData;
    buildChart(curDrop.chartData);

    //store data locally
    sessionStorage.setItem(curDrop.name, JSON.stringify(curDrop.chartData));

    $("#loading-div").hide();
    
    // Data is now available to download
    $("#download-div").show();
  }).fail(function(jqXHR, textStatus, errorThrown) {
    // If fail
    console.log(textStatus + ': ' + errorThrown);
  });
}

// Creates a hidden link and opens it to download the drop's data
// as a CSV file
function downloadCsv(){
  let curDrop = m_DropDataList[m_activeDropIndex]
  // split the comma sep data to insert new lines instead
  const rows = curDrop.chartData;
  let csvContent = "data:text/csv;charset=utf-8,";
  csvContent += "g\r\n";
  for(let i=0; i<rows.length; i++){
    csvContent += rows[i] + "\r\n";
  }

  // Create a hidden ele and click it to download the data
  var encodedUri = encodeURI(csvContent);
  var link = document.createElement("a");
  link.setAttribute("href", encodedUri);
  link.setAttribute("download", "my_data.csv");
  document.body.appendChild(link); // Required for FF

  link.click(); // This will download the data file named "my_data.csv".

}

//Delete the drop on confirmed delete
//TODO: Delete drop inthe middle messes up cur indexing
function deleteDrop(){
  let dropName = m_DropDataList[m_activeDropIndex].name;
  // Remove tab of item 
  $("#Drop-"+dropName).remove();
  $("#drop-details-container").hide();
  // $("#confirm-delete").prop("disabled", true).addClass("is-loading");
  // $("#close-delete-modal").prop("disabled", true);
  
  $("#modal-delete-btn").hide();
  $("#modal-delete-btn-loading").show();
  ajaxDeleteDrop();

}

function ajaxDeleteDrop(){
  let dropName = m_DropDataList[m_activeDropIndex].name;
  let requestString = urlEndPoint+"/recordDelete/"+dropName;
  return $.ajax({
            dataType: 'text',
            url: requestString
          }).done(function(response) {
            // If successful
            //remove from local storage
            sessionStorage.removeItem(dropName);
            // remove it from the global list of drops
            if (m_activeDropIndex > -1) {
                m_DropDataList.splice(m_activeDropIndex, 1);
            }

            //set active drop to null and hide the chart
            m_activeDropIndex = null;

            //Close modal
            closeModal();

            console.log("Delete Status: " + response);
              
          }).fail(function(jqXHR, textStatus, errorThrown) {
            // If fail
            console.log(textStatus + ': ' + errorThrown);
          });
}

// Show confirm delete dialog 
function openConfirmDeleteDialog(){
	buildModal();
	$(".modal").addClass("is-active");

}

// Reset and close the open modal
function closeModal() {
    // Set modal buttons back to their open state
    $("#modal-delete-btn").show();
    $("#modal-delete-btn-loading").hide();

		// $(".modal").removeClass("is-active");
		// $("#modal-area").val("");

    $("#my-modal").hide();
}

function createDrop(name, valuesArray){
	return {name: name, chartData: valuesArray};
}

//Pull all existing drop names from the egg and populate the tabs with them
function updateDropList() { 
  //Get all Drops from API
  ajaxUpdateDropList().then(LoadAllDropData);  
}

// Checks the drops in order to see if the drop's data exists locally. If it does then
// then skip pulling its data. If it does not then pull all following drop's data.
function LoadAllDropData(){
  for(let i=0; i<m_DropDataList.length; i++){
    let curDrop = m_DropDataList[i];
    let sessionDropData = JSON.parse(sessionStorage.getItem(curDrop.name));
    if(sessionDropData != null)
    {
      // Drop data exists locally
      curDrop.chartData = sessionDropData;

      // Clear loading flag
      curDrop.loading = false;
    } else {
      // Selectively load drop data
      m_loadingList.push(curDrop);
    }
  }

  // Initialize waterfall ajax call
  ajaxLoadAllDropData(m_loadingList.shift());
}

// Get Data for each drop and store it in session storage
function ajaxLoadAllDropData(drop) {
  let getDataUrl = urlEndPoint+"/recordGetMagnitudes/" + drop.name;

  return $.ajax({
    dataType: 'text',
    url: getDataUrl
  }).done(function(data) {
    //Plot data
    let magnitudeData = parseDataMagnitude(data);
    drop.chartData = magnitudeData;

    // Clear loading flag
    drop.loading = false;

    // Show the data if it's currently selected
    if(drop == m_DropDataList[m_activeDropIndex]) {
      buildChart(drop.chartData);
      $("#loading-div").hide();
      $("#download-div").show();
    }

    //Store data locally
    sessionStorage.setItem(drop.name, JSON.stringify(drop.chartData));

    if(m_loadingList.length > 0) {
      ajaxLoadAllDropData(m_loadingList.shift());
    }
  }).fail(function(jqXHR, textStatus, errorThrown) {
    // If fail
    console.log(textStatus + ': ' + errorThrown);
    alert("Error loading data from egg");
  });
}

// Gets all drop names and creates tabs for each one.
function ajaxUpdateDropList() {
  let requestStr = urlEndPoint+"/recordList"
  return  $.ajax({
      dataType: 'text',
      url: requestStr
    }).done(function(response) {
      // remove new line chars from right
      response = response.substring(0,response.length-1);
      //Check that names exist to avoid adding empty tabs
      console.log("recordList:\n" + response);
      if(response != ""){
        let dropNames = response.split('\n');
        for(let i=0; i<dropNames.length; i++){
          // Add a drop and a tab with no data because it hasn't been pulled yet
          addDrop(dropNames[i], []);

          // Set loading flag
          m_DropDataList[i].loading = true;
        }
      }
    }).fail(function(jqXHR, textStatus, errorThrown) {
      // If fail
      console.log(textStatus + ': ' + errorThrown);
    });
}

function getVersion() {
  let requestStr = urlEndPoint+"/getVersion"
  return  $.ajax({
    dataType: 'text',
    url: requestStr
  }).done(function(response) {
    document.getElementById("ver").innerHTML = "SmartEgg Version: " + response;
  }).fail(function(jqXHR, textStatus, errorThrown) {
    // If fail
    console.log(textStatus + ': ' + errorThrown);
  });
}

function getBattLevel() {
  let requestStr = urlEndPoint+"/getBatteryLevel"
  return  $.ajax({
    dataType: 'text',
    url: requestStr
  }).done(function(response) {
    document.getElementById("battlevel").innerHTML = "Battery Level: " + response + "%";
  }).fail(function(jqXHR, textStatus, errorThrown) {
    // If fail
    console.log(textStatus + ': ' + errorThrown);
  }).always(function(data, textStatus, errorThrown) {
    //setTimeout(getBattLevel, 5000);
  });
}

function getAPName() {
  let requestStr = urlEndPoint+"/getAPName"
  return  $.ajax({
    dataType: 'text',
    url: requestStr
  }).done(function(response) {
    document.getElementById("apname").innerHTML = "AP Name: " + response;
  }).fail(function(jqXHR, textStatus, errorThrown) {
    // If fail
    console.log(textStatus + ': ' + errorThrown);
  });
}

function isConnected() {
  let requestStr = "/gen_204"
  return  $.ajax({
    dataType: 'text',
    url: requestStr
  }).done(function(response) {
    // Check if any datasets are downloading
    let loadingData = false;
    for(let e of m_DropDataList) {
      if(e.loading) {
        loadingData = true;
        break;
      }
    }

    if(loadingData) {
      document.getElementById("connstat").innerHTML = "Status: Loading data";
    } else {
      document.getElementById("connstat").innerHTML = "Status: Ready";
    }
  }).fail(function(jqXHR, textStatus, errorThrown) {
    document.getElementById("connstat").innerHTML = "Status: Disconnected";
    // If fail
    console.log(textStatus + ': ' + errorThrown);
  }).always(function(data, textStatus, errorThrown) {
    setTimeout(isConnected, 3000);
  });
}

// Adds a drop to the drop list and the tabs
function addDrop(dropName, data) {
  m_nextDropId += 1;
  let eggDrop = {id: m_nextDropId, name: dropName, chartData: data, loading: false };
  m_DropDataList.push(eggDrop);
  this.addTab(eggDrop.id, dropName);
}

// Returns a bool if the newName parameter is unique in the m_DropDataList of names
function nameIsUnique(newName){
  for(let i=0; i<m_DropDataList.length; i++){
    if(m_DropDataList[i].name == newName){
      return false;
    }
  }
  return true;
}

// Check that the drop name is valid. Returns the validity of the name.
// remarks: the drop name may not have special characters, spaces or be empty
function nameIsValid(name){
  let isValid = true;

  // string can not have spaces or special characters or be empty
  if(name.indexOf(' ') >= 0 || /[~`!#$%\^&*+=\-\[\]\\';,/{}|\\":<>\?]/g.test(name) || name.length == 0 ){
      isValid = false;
  }

  return isValid;
}

// Add a drop to the list of tabs
function addTab(dropId, dropName){
	$("#drop-history").append('<li id=Drop-'+dropName+'> <a onClick="newDataSelected('+dropId+')">'+dropName+'</a></li>');
}

// Add valid, unique names to list of tabs. 
function addNewDrop() {
  let dropName = $("#drop-name").val();

  // check that the name is unique
  let nameIsUnique = this.nameIsUnique(dropName);
  let nameIsValid = this.nameIsValid(dropName);

  if(!nameIsValid) {
    // Name has invalid space in it
    alert("The name " + dropName + " is invalid. Please remove all spaces and special characters from the name.");
  }
  else if(!nameIsUnique){
    // Name is not unique, show error message
    alert("The name " + dropName + " is already used. Please use a unique name.");    
  } else {
    // Name is valid
    $("#drop-name").val("");
    let dropId = m_DropDataList.length;
    let curDataList = []; //empty data list to start  
    this.addDrop(dropName, curDataList);
  }
}

// Builds modal for delete confirmation
function buildModal() {
	let dropName = m_DropDataList[m_activeDropIndex].name;
    $("#my-modal").show();
    $("#modal-card-content").text("Are you sure you'd like to delete the drop " + dropName + "? There is no undoing this operation. ");
	// $("#modal-area").append('<div id="my-modal" class="modal">\
	// 	<div class="modal-background"></div>\
	// 		<div class="modal-card">\
	// 			<header class="modal-card-head">\
	// 				<p class="modal-card-title">Confirm Delete</p>\
	// 				<button class="delete" aria-label="close" onClick="closeModal()"></button>\
	// 			</header>\
	// 			<section class="modal-card-body">\
	// 				<!-- Content ... -->\
	// 				Are you sure you\'d like to delete the drop "' + dropName + '"? There is no undoing this operation. \
	// 			</section>\
	// 			<footer class="modal-card-foot">\
	// 				<div id="modal-delete-btn">\
 //            <button id="confirm-delete" class="button is-danger" onClick="deleteDrop()">Delete Drop</button>\
 //  					<button id="close-delete-modal" class="button" onClick="closeModal()">Cancel</button>\
 //          </div>\
 //          <div id="modal-delete-btn-loading">\
 //            <button id="confirm-delete-loading" class="button is-danger is-loading" onClick="deleteDrop()">Delete Drop</button>\
 //            <button id="close-delete-modal-loading" class="button" disabled="true" onClick="closeModal()">Cancel</button>\
 //          </div>\
	// 			</footer>\
	// 		</div>\
	// 	</div>');
}

// Loads data and sets gui visuals for the selected tab
function newDataSelected(dropId){
    //Toggle active tabs
    if(m_activeDropIndex != null) {
        // if tab is selected remove the old tabs active calss
        $("#Drop-"+m_DropDataList[m_activeDropIndex].name).removeClass("is-active");
    }

    let curDropIndex = this.getDropIndex(dropId);

    $("#Drop-"+m_DropDataList[curDropIndex].name).addClass("is-active");
    m_activeDropIndex = curDropIndex;

    $("#drop-details-container").show()

    console.log("Selected Tab: " + m_DropDataList[curDropIndex].name);
    selectedData = m_DropDataList[curDropIndex].chartData;

    //Hide start button if the drop has already been recorded
    if(m_DropDataList[curDropIndex].loading == true) {
      $("#loading-div").show();
      $("#download-div").hide();
      $("#record").hide();
    } else if (selectedData.length == 0){
      // No data has been cached for this drop
      $("#loading-div").hide();
      $("#download-div").hide();
      $("#record").show();
    } else {
      // Data has been recorded for this drop
      $("#loading-div").hide();
      $("#download-div").show();
      $("#record").hide();
    }
    let currentChart = buildChart(selectedData)
}

// Returns the index of the first drop with the id provided
function getDropIndex(dropId){
      // Find the drop by the id
    for(let i=0; i<m_DropDataList.length; i++ )
    {
      let curDrop = m_DropDataList[i];
      if(curDrop.id == dropId)
      {
        return i;
      }
    }
    //Error if here
    return 0;
}

function parseDataMagnitude(data) {
  let dataListToPlot = [];

  //Split the result string on all new line chars
  resultList = data.split('\n');

  //Iterate through each collected data set Skip the first header row
  for(let i=1; i<resultList.length; i++){
    let curDataSet = resultList[i];
    //split the data point into its individual x, y, z components
    let curDataPoints = curDataSet.split(",");

    // TODO: add time back in
    // Find the magnitude
    let tPoint = parseFloat(curDataPoints[0]);
    let yPoint = parseFloat(curDataPoints[1]);

    //let dataPoint = [tPoint, yPoint];

    dataListToPlot.push(yPoint);
  }

  return dataListToPlot;
}

// Builds a line chart for the supplied y data array
// ResultData: The y values of the line chart
function buildChart(resultData) {
    let chart = Highcharts.chart('charting-container', {
            chart: {
                type: 'line',
                zoomType: 'x'
            },
            title: {
                text: 'Smart Egg Force Graph'
            },
        
            scrollbar: {
                enabled: true
            },
            xAxis: {
                title: {
                    text: 'Sample Number'
                }
            },
            yAxis: {
                title: {
                    text: 'Force(g)'
                }
            },
            legend: {
                layout: 'vertical',
                align: 'right',
                verticalAlign: 'middle'
            },
            series: [{
                name: 'Force (g)',
                data: resultData
            }],
        
            responsive: {
                rules: [{
                    condition: {
                        // maxWidth: 10000
                    },
                    chartOptions: {
                        legend: {
                            layout: 'horizontal',
                            align: 'center',
                            verticalAlign: 'bottom'
                        }
                    }
               
            }]
        }
    });

    return chart;
}
/* 
 *  Keep compatibility with multiple web browsers
 *  I'm looking at you Internet Explorer
 */
function makeHTTPObject() {
  try {return new XMLHttpRequest();}
  catch (error) {}
  try {return new ActiveXObject("Msxml2.XMLHTTP");}
  catch (error) {}
  try {return new ActiveXObject("Microsoft.XMLHTTP");}
  catch (error) {}

  throw new Error("Could not create HTTP request object.");
}

/* Make a new Http Request */
function HTTPRequest(url, successFunction) {
  var request = makeHTTPObject();

  request.onreadystatechange = function() {
    if(request.readyState == 4) {
      if(request.status == 200) {
        successFunction(request.responseText);
      } else {
        /* Return an empty string if there's no data */
        throw new Error("Empty Data in HTTP Request");
      }
    }
  };

  request.open("GET", url, true);
  request.send(null);
}

// Some Available api calls
// /functions/recordStart/<recordingName>           // Starts a recording with the name <recordingName>, returns string -1 if no name provided, -2 if the code timed out, -3 if name already exists, -4 if no space
// /functions/recordStop                                                  // Stops a current recording and returns 0 if successful, -1 if no recording in progress
// /functions/recordStatus                                                               // Returns 0 if idle, 1 if recording
// /functions/recordList                                                     // Returns list of recording separated by new line character ‘\n’
// /functions/recordGetRaw/<recordingName>    // Chunked get response containing a csv string (~620KB) with raw datavalues (0 to 4096), content contains “t,x,y,z”
// /functions/recordGetForce/<recordingName>  // Chunked get response containing a csv string (~620KB) with gForce data (-200 to 200 G), content contains “t,x,y,z”
// /functions/recordDelete/<recordingName>       // Deletes a recording with the name <recordingName>, returns string 0 if successful, -1 if does not exist
// /functions/recordDeleteAll                                         // Deletes all recordings
// /functions/recordSpaceLeft                                        // Returns string with size in bytes left
// /functions/recordGetMaxTime                                 // Returns string with max recording time (Default 60 Seconds)
// /functions/recordSetMaxTime/<timeInSeconds>            // Set the max recording time
// /functions/calibrate                                                        // Calibrates the accelerometer and returns a string with the offsets