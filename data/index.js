/**
 * ----------------------------------------------------------------------------
 * Energy Monitor
 * ----------------------------------------------------------------------------
 * © 2024 Hans Weda
 * ----------------------------------------------------------------------------
 */

var chart_power;
var chart_ipi;
var chart_raw;

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

function onLoad(event) {

    // initialize chart and events
    initChart();
    initEvents();

    // initialize the data
    getData(function (xhttp) {
        updateRawGraph(xhttp);
        initEnergyGraphData(xhttp);
    } );
    
    // refresh the data if the tab is visible again
    document.addEventListener("visibilitychange", () => {
        if (document.visibilityState == "visible") {
            console.log("refresh data");
            
            // remove the data
            chart_raw.series[0].setData();
            chart_raw.series[1].setData();
            
            // add new data
            getData(function (xhttp) {
                updateRawGraph(xhttp);
                initEnergyGraphData(xhttp);
            } );
        }
    });
    
    // regularly update the raw data
    setInterval(function () {
        getData(updateRawGraph)
    }, 5000 ) ; 
}

// ----------------------------------------------------------------------------
// Define Chart
// ----------------------------------------------------------------------------


function initChart() {

    // define raw-data chart
    chart_raw = new Highcharts.Chart({
      chart:{ renderTo : 'chart-raw-data' },
      title: { text: 'Ruwe signaal' },
      series: [{
        name: 'Ruwe signaal',
        type: 'line',
        color: '#059e8a',
        data: []
      }, {
        name: 'Peaks',
        type: 'scatter',
        color: '#9e058a',
        data: []
      }],
      xAxis: { type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Lichtintensiteit [a.u.]' }
      },
      credits: { enabled: false }
    });
    
    // define inter-peak-interval chart
    chart_ipi = new Highcharts.Chart({
      chart:{ renderTo : 'chart-ipi' },
      title: { text: 'Interval tussen pieken' },
      series: [{
        name: 'IPI',
        type: 'line',
        color: '#059e8a',
        marker: {
            enabled: true
        },
        data: [],
        showInLegend: false
      }],
      xAxis: { type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Inter-Peak-Interval [ms]' }
      },
      credits: { enabled: false }
    });   

    // define power chart
    chart_power = new Highcharts.Chart({
      chart:{ renderTo : 'chart-power' },
      title: { text: 'Vermogen' },
      series: [{
        name: 'Vermogen',
        type: 'line',
        color: '#059e8a',
        marker: {
            enabled: true
        },
        data: [],
        showInLegend: false
      }],
      xAxis: { type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Vermogen [W]' }
      },
      credits: { enabled: false }
    });
}


// ----------------------------------------------------------------------------
// Define Graph updates
// ----------------------------------------------------------------------------

function initEnergyGraphData(xhttp) {
    // get the peaks data from xhttp request
    const peaks = xhttp.response["peaks"];
    
    // define variables
    var ipi = [];
    var power = [];
    
    // fill variables by peak data
    for (let i = Math.max(1, peaks.length-10); i < peaks.length; i++) {
        ipi.push([ peaks[i][0], peaks[i][0] - peaks[i-1][0] ]);
        power.push([ peaks[i][0], ipi2power( peaks[i][0] - peaks[i-1][0] ) ]);
    };
    
    // update charts
    chart_ipi.series[0].setData(ipi);
    chart_power.series[0].setData(power);
}


function updateEnergyGraph(event) {
    // add inter-peak-intervals
    let data = JSON.parse(event.data);
    if ( data["ipi"] > 0 ) {

        // only add data if there is a difference
        // add to ipi chart
        if(chart_ipi.series[0].data.length > 10) {
            chart_ipi.series[0].addPoint([ data["timestamp"], data["ipi"] ], true, true, true);
        } else {
            chart_ipi.series[0].addPoint([ data["timestamp"], data["ipi"] ], true, false, true);
        }

        // add to power chart
        if(chart_power.series[0].data.length > 10) {
            chart_power.series[0].addPoint([ data["timestamp"], ipi2power( data["ipi"] ) ], true, true, true);
        } else {
            chart_power.series[0].addPoint([ data["timestamp"], ipi2power( data["ipi"] ) ], true, false, true);
        }

    }
}


function updateRawGraph(xhttp) {
    // add raw signal data
    const data = xhttp.response["data"];
    for (let i = 0; i < data.length; i++) {
        if(chart_raw.series[0].data.length > 1200) {
            chart_raw.series[0].addPoint(data[i], false, true, true);
        } else {
            chart_raw.series[0].addPoint(data[i], false, false, true);
        }
    };
      
    // add detected peaks to raw signal data
    const peaks = xhttp.response["peaks"];
    // remove existing data
    chart_raw.series[1].setData([]);
    // only add peaks to visible raw signal
    for (let i = 0; i < peaks.length; i++) {
        if(peaks[i][0] >= chart_raw.xAxis[0].getExtremes().dataMin) {
            chart_raw.series[1].addPoint(peaks[i], false);
        }
    };
      
    // redraw the raw-data chart
    chart_raw.redraw();
}


function getData(callback) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            callback(this)
        }
    };
    xhttp.open("GET", "/data", true);
    xhttp.responseType = "json";
    xhttp.send();
}

// ----------------------------------------------------------------------------
// Define Events
// ----------------------------------------------------------------------------


function initEvents() {
    // check if the browser supports EventSource or not
    if (!!window.EventSource) {
      var source = new EventSource('/events');
      
      source.addEventListener('open', function(e) {
        console.log("Events Connected");
      }, false);
      
      source.addEventListener('error', function(e) {
        if (e.target.readyState != EventSource.OPEN) {
          console.log("Events Disconnected");
        }
      }, false);

      source.addEventListener('message', function(e) {
        console.log("message", e.data);
      }, false);

      source.addEventListener('peak', function(e) {
        console.log("peak", e.data);
        updateEnergyGraph(e);
      }, false);

    }
}

// ----------------------------------------------------------------------------
// Calculate power from inter-peak-interval
// ----------------------------------------------------------------------------

function ipi2power(ipi) {
    // calculate power from inter peak interval
    // assuming 1 peak per 1 Wh energy usage
    // this equals power = (3600 / x [sec]) Watts
    // or power = (3600000 / x [ms] ) Watts
    return (3600000 / ipi)
}
