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
    var chartT = new Highcharts.Chart({
      chart:{ renderTo : 'chart-energy' },
      title: { text: 'Energie verbruik' },
      series: [{
        showInLegend: false,
        data: []
      }],
      plotOptions: {
        line: { animation: false,
          dataLabels: { enabled: true }
        },
        series: { color: '#059e8a' }
      },
      xAxis: { type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Energie (Wh)' }
      },
      credits: { enabled: false }
    });
    
    setInterval(function ( ) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var x = (new Date()).getTime(),
              y = parseFloat(this.responseText);
          //console.log(this.responseText);
          if(chartT.series[0].data.length > 40) {
            chartT.series[0].addPoint([x, y], true, true, true);
          } else {
            chartT.series[0].addPoint([x, y], true, false, true);
          }
        }
      };
      xhttp.open("GET", "/energy", true);
      xhttp.send();
    }, 30000 ) ;    
}






