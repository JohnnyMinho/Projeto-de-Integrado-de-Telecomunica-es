// index.js


var usersRouter = require('./BaseDados');
var loginRouter = require('./login');
var registerRouter = require('./Register');
var DashBoardRouter = require('./AdminDashBoard');

const {body,validationResult} = require('express-validator');
var cookieParser = require('cookie-parser');
var db = require('./Database');
/**
 * Required External Modules
 */ 
const express = require("express");
const path = require("path");
/**
 * App Variables
 */

const app = express();
const port = process.env.PORT || "8000";

/**
 *  App Configuration
 */
 
app.set("views", path.join(__dirname, "views"));
app.set("view engine", "pug");
app.use(express.static(path.join(__dirname, "public")));
var bodyParser = require('body-parser');
const { builtinModules } = require('module');

//parse requests
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(cookieParser());

//Autenticação
var session = require('express-session');
var MySQLStore = require('express-mysql-session')(session);
var passport = require('passport'); 
var LocalStrategy = require('passport-local');
var bcrypt = require('bcrypt');
var userpriviliges;

passport.use(new LocalStrategy(
  function(username,password, done){
      console.log(username);
      console.log(password);
      db.query('SELECT * FROM Users WHERE Email = ?', [username],function (err,results,fields){
        if (err){
          return done(err);
        }
        else{
        if(results.length === 0){
          return done(null,false);
        }
        const hash = results[0].Senha.toString();
        const user = results[0].idUser;
        if(results[0].Role === "Admin"){
          userpriviliges = true;
        }else{
          userpriviliges = false;
        }
        bcrypt.compare(password, hash, function(err,response){
          if(response){
            return done(null, {user});
          } else{
            return done(null,false);
          }
        })
        
      }
      })
  }) 
);

var options = {
  host: "localhost",
  user: "root",
  port:3306,
  password: "Sins@741",
  database:'tabeladb'
};

var sessionStore = new MySQLStore(options);

app.use(session({
  secret: 'asdjadhwh2asdkmwbi',
  store: sessionStore,
  resave: false,
  saveUninitialized: false,
  //cookie: { secure: true }
}));

app.use(passport.initialize());
app.use(passport.authenticate('session'));

//Função para dizer se está autenticado às views e se é admin ou não

app.use(function(req,res,next){
  res.locals.isAuthenticated = req.isAuthenticated();
  res.locals.Usertype = req.userpriviliges;
  next();
});

app.use(function(req,res,next){
  res.locals.Usertype = userpriviliges;
  console.log(userpriviliges);
  next();
});
/**
 * Routes Definitions
 */
 
app.get("/", (req, res) => {
  res.render("index", { title: "Home" });
});
app.get("/index", (req, res) => {
  res.render("index", { title: "Home" });
});

app.get('/Equipa',authenticationMiddleware() ,(req, res) => {
  res.render("Equipa", {title: "Equipa"});
});

app.get('/relatorios',authenticationMiddleware() ,(req, res) => {
  res.render("relatorios", {title: "Relatorios"});
});

app.get('/logout', (req,res) =>{
  req.logout();
  req.session.destroy();
  userpriviliges = false;
  res.redirect("/index");
});


/*app.get('/dados', (req, res) => {
  res.render("dados", {title: "Dados"});
  //res.status(301).redirect("dados");
});*/

app.use('/dados', usersRouter);

app.use('/login', loginRouter);

app.use('/register',registerRouter);

app.use('/AdminDashBoard', DashBoardRouter);
/**
 * Server Activation
 
 */
 app.listen(port,'0.0.0.0', () => {
  console.log(`Listening to requests on http://localhost:${port}`);
});

passport.serializeUser(function(id_user, cb) {
  cb(null,id_user);
});

passport.deserializeUser(function(id_user, cb) {
  cb(null,id_user);
});

function authenticationMiddleware() {  
	return (req, res, next) => {
		console.log(`req.session.passport.user: ${JSON.stringify(req.session.passport)}`);

	    if (req.isAuthenticated()) return next();
	    res.redirect('/login')
	}
}

const checkpriviliges = (...Role) => (req,res,next) =>{
  if(!req.user){
    return false;
  }
}



module.exports = app;