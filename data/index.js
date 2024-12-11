/**
 * ----------------------------------------------------------------------------
 * Energy Monitor
 * ----------------------------------------------------------------------------
 * © 2024 Hans Weda
 * ----------------------------------------------------------------------------
 */

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

function onLoad(event) {
  initChart();
}

// ----------------------------------------------------------------------------
// Define Chart
// ----------------------------------------------------------------------------


function initChart() {

    // define raw-data chart
    var chart_raw = new Highcharts.Chart({
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
    
    // define inter-peak-inteval chart
    var chart_energy = new Highcharts.Chart({
      chart:{ renderTo : 'chart-energy' },
      title: { text: 'Energie verbruik' },
      series: [{
        name: 'IPI',
        type: 'line',
        color: '#059e8a',
        marker: {
            enabled: true
        },
        data: []
      }],
      xAxis: { type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'IPI' }
      },
      credits: { enabled: false }
    });

    
    setInterval(function ( ) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          
          // add raw signal data
          const data = this.response["data"];
          for (let i = 0; i < data.length; i++) {
            if(chart_raw.series[0].data.length > 1200) {
                chart_raw.series[0].addPoint(data[i], false, true, true);
            } else {
                chart_raw.series[0].addPoint(data[i], false, false, true);
            }
          };
          
          // add detected peaks to raw signal data
          const peaks = this.response["peaks"];
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
          
          // add inter-peak-intervals
          var ipi = [];
          for (let i = 1; i < peaks.length; i++) {
            if ( peaks[i][0] > peaks[i-1][0] ) {
                // only add data if there is a difference
                ipi.push([ peaks[i][0], peaks[i][0] - peaks[i-1][0] ]);
            }
          }
          chart_energy.series[0].setData(ipi);
        }
      };
      xhttp.open("GET", "/data", true);
      xhttp.responseType = "json";
      xhttp.send();
    }, 5000 ) ;    
}


