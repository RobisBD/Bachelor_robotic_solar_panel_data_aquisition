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

  client.publish(
    "server/api-data",
    JSON.stringify({ cloudy: 1, sun_lat: , sun_lng:  })
  );
});

