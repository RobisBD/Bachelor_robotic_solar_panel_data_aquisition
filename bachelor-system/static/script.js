const ctx = document.getElementById("sensors");
const ctx2 = document.getElementById("weather");
const ctx3 = document.getElementById("sun-traject");
const ctx4 = document.getElementById("radiation");
const OPEN_WEATHER_API_KEY = "ca9cb638da346217fe6a1a21ab0e1b6f";
const OPEN_WEATHER_API_WEATHER_URL =
  "https://api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}&exclude={part}&appid={API key}";
const OPEN_WEATHER_API_SOLAR_RADIANCE_URL =
  "https://api.openweathermap.org/data/2.5/solar_radiation?lat={lat}&lon={lon}&appid={API key}"; // Docs: https://openweathermap.org/api/solar-radiation

async function getWeatherData() {
  const JSONData = await fetch(
    `https://api.openweathermap.org/data/2.5/weather?lat=${56.955537}&lon=${24.108819}&appid=${OPEN_WEATHER_API_KEY}`
  );
  const data = await JSONData.json();
  console.log(data);
}
getWeatherData();

let map = L.map("map").setView([56.955537, 24.108819], 13);
L.tileLayer("https://tile.openstreetmap.org/{z}/{x}/{y}.png", {
  maxZoom: 19,
  attribution:
    '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>',
}).addTo(map);
// let marker1 = L.marker([56.9658, 24.1095]).addTo(map);
// let marker2 = L.marker([56.9657429, 24.111836]).addTo(map);
// let marker3 = L.marker([56.9642339, 24.1094112]).addTo(map);
// let marker4 = L.marker([56.9641637, 24.1116643]).addTo(map);

const pwrChart = new Chart(ctx, {
  type: "line",
  data: {
    datasets: [
      {
        label: "Saražotā enerģija",
        borderWidth: 1,
        backgroundColor: "rgba(163, 69, 161, 0.2)",
        borderColor: "rgba(163, 69, 161, 1)",
      },
    ],
  },
  options: {
    scales: {
      y: {
        beginAtZero: true,
        title: {
          display: true,
          align: "center",
          text: "Enerģija (mW)",
          color: "#45a367",
          font: {
            size: 16,
          },
        },
      },
      x: {
        title: {
          display: true,
          align: "center",
          text: "Laiks",
          color: "#45a367",
          font: {
            size: 16,
          },
        },
      },
    },
    fill: true,
  },
});
const cloudChart = new Chart(ctx2, {
  type: "line",
  data: {
    datasets: [
      {
        label: "Mākoņu daudzums",
        borderWidth: 1,
        backgroundColor: "rgb(163, 135, 69, 0.2)",
        borderColor: "rgb(163, 135, 69, 1)",
      },
    ],
  },
  options: {
    scales: {
      y: {
        beginAtZero: true,
        title: {
          display: true,
          align: "center",
          text: "Mākoņu daudzums (%)",
          color: "#45a367",
          font: {
            size: 16,
          },
        },
      },
      x: {
        title: {
          display: true,
          align: "center",
          text: "Laiks",
          color: "#45a367",
          font: {
            size: 16,
          },
        },
      },
    },
    fill: true,
  },
});
const trajectoryChart = new Chart(ctx3, {
  type: "line",
  data: {
    datasets: [
      {
        label: "Saules trajektorija",
        borderWidth: 1,
      },
    ],
  },
  options: {
    scales: {
      y: {
        beginAtZero: true,
      },
    },
  },
});
const irradianceChart = new Chart(ctx4, {
  type: "line",
  data: {
    datasets: [
      {
        label: "Saules UV līmenis",
        borderWidth: 1,
        backgroundColor: "rgb(163, 89, 69, 0.2)",
        borderColor: "rgb(163, 89, 69, 1)",
      },
    ],
  },
  options: {
    scales: {
      y: {
        beginAtZero: true,
        title: {
          display: true,
          align: "center",
          text: "Saules UV staru līemnis",
          color: "#45a367",
          font: {
            size: 16,
          },
        },
      },
      x: {
        title: {
          display: true,
          align: "center",
          text: "Laiks",
          color: "#45a367",
          font: {
            size: 16,
          },
        },
      },
    },
    fill: true,
  },
});

const robots = document.querySelectorAll(".robot-card");
const spinnerOverlay = `<div class="overlay"><div id='loading'></div></div>`;
let robotData;

robots.forEach((robot) => {
  robot.addEventListener("click", async function (e) {
    if (
      e.target.classList.contains("offline-robot") ||
      e.target.classList.contains("viewing")
    )
      return;
    // display loading spinner
    document.body.insertAdjacentHTML("beforeBegin", spinnerOverlay);

    // get data from database
    const dataJSON = await fetch("http://10.10.10.175:3000/data");
    const data = await dataJSON.json();
    robotData = data;
    console.log(data);

    // display robot data (lat, lng, battery) from last readings
    renderRobotData(e.target, robotData[0]);

    // set map view to last received
    map.setView([robotData[0].location_lat, robotData[0].location_lng], 17);

    // add markers to map
    displayMarkers();

    // filter data into seperate arrays for charting
    splitDataDisplayCharts();

    // get sun trajectory data from API

    // display charts
    e.target.classList.add("viewing");
    document.querySelector(".overlay").remove();
  });
});

function displayMarkers(markerCount = 6) {
  const polyline = [];
  for (let i = 0; i <= markerCount; i++) {
    const lat = robotData[i].location_lat;
    const lng = robotData[i].location_lng;
    polyline.push([lat, lng]);
    L.marker([lat, lng]).addTo(map);
    L.polyline(polyline, { color: "red" }).addTo(map);
  }
}

function renderRobotData(target, obj) {
  target.querySelector(".lat").textContent = `${String(obj.location_lat).slice(
    0,
    8
  )}`;
  target.querySelector(".lng").textContent = `${String(obj.location_lng).slice(
    0,
    8
  )}`;
  target.querySelector(
    ".battery-level"
  ).innerHTML = `<i class="bx bx-battery"></i>${obj.battery_level}%`;
}

function splitDataDisplayCharts(pointCount = 6) {
  const timeArr = [];
  const avrgPwrArr = [];
  const cloudinessArr = [];
  const irradianceArr = [];

  for (let i = 0; i < pointCount; i++) {
    const currRobot = robotData[i];
    const uploadObj = new Date(currRobot.upload_time);
    const uploadTime = uploadObj.toLocaleTimeString("lv-LV", { hour12: false });

    timeArr.push(uploadTime);
    avrgPwrArr.push(currRobot.power_avg);
    cloudinessArr.push(+currRobot.api_data_details[0].cloudiness_prcntg);
    irradianceArr.push(+currRobot.api_data_details[0].directNormalIrradiance);
  }
  // timeArr.splice(-2);
  console.log(timeArr, avrgPwrArr, cloudinessArr, irradianceArr);
  pwrChart.data.labels = timeArr;
  pwrChart.data.datasets[0].data = avrgPwrArr;
  pwrChart.update();

  cloudChart.data.labels = timeArr;
  cloudChart.data.datasets[0].data = cloudinessArr;
  cloudChart.update();

  // trajectoryChart.data.labels = timeArr;
  // trajectoryChart.data.datasets[0].data = avrgPwrArr;
  // trajectoryChart.update();

  irradianceChart.data.labels = timeArr;
  irradianceChart.data.datasets[0].data = irradianceArr;
  irradianceChart.update();
}
