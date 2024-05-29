const express = require("express");
const bodyParser = require("body-parser");
const mqtt = require("mqtt");
const robotRoutes = require("./routes/robotRoutes");
const sequelize = require("./database/db");

const RobotData = require("./models/robot");
const ApiDataDetail = require("./models/api");

const port = 3000;
const app = express();

app.set("view engine", "ejs");
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(express.static("static"));

app.use(robotRoutes);

RobotData.hasMany(ApiDataDetail);
ApiDataDetail.belongsTo(RobotData);

sequelize
  .sync()
  .then((res) => {
    app.listen(port);
  })
  .catch((err) => console.log(err));


const client = mqtt.connect("mqtt://10.10.10.175");

client.on("connect", (conn) => {
  console.log("Connected client!", conn);

  client.subscribe("esp32/data");
});

client.on("message", (topic, message, packet) => {
  console.log(topic);
  console.log(JSON.parse(packet.payload.toString()));

  const {sunAltitude, sunLatitude, cloudinessPrecentage} = getWeatherData("Riga", "09ffsf0944f40909090#44fe");
  
  client.publish(
    "server/api-data",
    JSON.stringify({ cloudy: cloudinessPrecentage, sun_lat: sunLatitude, sun_lng: sunAltitude })
  );
});

async function getWeatherData(cityName, apiKey) {
    const apiUrl = `https://api.openweathermap.org/data/2.5/weather?q=${cityName}&appid=${apiKey}`;
    
    try {
        const response = await fetch(apiUrl);
        const data = await response.json();
        
        if (response.ok) {
            const sunAltitude = data.sys.sunrise ? data.sys.sunrise : "N/A";
            const sunLatitude = data.coord.lat ? data.coord.lat : "N/A";
            const cloudinessPercentage = data.clouds.all ? data.clouds.all : "N/A";
            
            return {
                sunAltitude,
                sunLatitude,
                cloudinessPercentage
            };
        } else {
            throw new Error(data.message);
        }
    } catch (error) {
        console.error("Error fetching weather data:", error);
        return null;
    }
}
