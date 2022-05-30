var express = require('express');
var router = express.Router();
var login = require('./loginauth');
var db=require('./Database');
var app2 = require('./index');
const {body,validationResult} = require('express-validator');
const bcrypt = require('bcrypt');
const saltRounds = 10;
var passport = require('passport'); 

router.get('/', function (req, res, next) {
    res.render("register", {title: "Register"});
    //res.status(301).redirect("Register");
});
/* Login user */
router.post('/', function (req, res, next) {const username = req.body.username;
    const name = req.body.name;
    const password = req.body.password;
    var role = "User";
    console.log(req.body.Role);
    if(req.body.Role){
        role = "Admin";
    }else{
        role = "User";
    }
    let loginResult = login(username, password);

    body('name').notEmpty();
    body('password').isLength({min: 5},{max: 20});
    body('username').isEmail();
    console.log(req.body.passwordc);
    body('password').custom((value, {req}) =>{
        if(value !== req.body.passwordc){
            throw new Error('Passwords dont match');
        }
        return true
    });
    const errors = validationResult(req);
    if(!errors.isEmpty){
        console.log(`erros: ${JSON.stringify(errors)}`);
        res.render("register",{title: "ERRO NO REGISTO", "erros" : errors});
    }
    bcrypt.hash(password, saltRounds, function(err, hash) {
        if (!loginResult && errors.isEmpty) {
            var newuser = {
                "Nome" : name,
                "Senha" : hash,
                "Email" : username,
                "Role"  : role
            };
            db.query('INSERT INTO Users SET ?' , newuser, function(err, resp) {
                if(err){
                    throw err
                }
                const id_user = resp.insertId
                req.login(id_user,function(err){
                    res.redirect('/index');
                });
            });
    }});
    passport.serializeUser(function(id_user, cb) {
        cb(null,id_user);
      });
      
      passport.deserializeUser(function(id_user, cb) {
       cb(null,id_user);
      });
    });
module.exports = router;