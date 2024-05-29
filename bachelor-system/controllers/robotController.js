const RobotData = require("../models/robot");
const ApiDataDetail = require("../models/api");

const robot_index = (req, res) => {
  res.render("index");
};
const robot_map = (req, res) => {
  console.log("map page");
};
const robot_data = async function (req, res) {
  const allRobotData = await RobotData.findAll({
    include: [
      {
        model: ApiDataDetail,
      },
    ],
    order: [["upload_time", "DESC"]],
  });

  res.json(allRobotData);
};
const robot_data_details = (req, res) => {
  console.log("data details page");
};

module.exports = {
  robot_index,
  robot_map,
  robot_data,
  robot_data_details,
};
