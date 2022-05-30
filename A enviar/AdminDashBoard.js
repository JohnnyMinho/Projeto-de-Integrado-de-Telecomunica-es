var express = require('express');
var router = express.Router();
var db=require('./Database');

router.get('/', (req,res) =>{
    res.render('Adminboard', {title: "DashBoard"})
});

module.exports = router;