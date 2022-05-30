var express = require('express');
var router = express.Router();
var login = require('./loginauth');/* GET users listing. */
var db = require('./Database');
var appindex = require('./index');
var passport = require('passport'); 

router.get('/', function (req, res, next) {
    res.render("login", {title: "Login"});
});

/* Se o usuário for autenticado com sucesso, ele é redirecionado para o index, senão volta à pagina*/
router.post('/', passport.authenticate('local', {
    successRedirect: '/index',
    failureRedirect: '/login'
}));

passport.serializeUser(function(id_user, cb) {
    cb(null,id_user);
  });
  
passport.deserializeUser(function(id_user, cb) {
    cb(null,id_user);
  });
module.exports = router;