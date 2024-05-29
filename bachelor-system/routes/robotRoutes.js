const express = require("express");
const robotController = require("../controllers/robotController");
const router = express.Router();

router.get("/", robotController.robot_index);

router.get("/map", robotController.robot_map);

router.get("/data", robotController.robot_data);

router.get("/data/:id", robotController.robot_data_details);

module.exports = router;
