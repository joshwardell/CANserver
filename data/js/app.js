//https://github.com/MatheusAvellar/textarea-line-numbers
const TLN={eventList:{},update_line_numbers:function(e,t){let n=e.value.split("\n").length-t.children.length;if(n>0){const e=document.createDocumentFragment();for(;n>0;){const t=document.createElement("span");t.className="tln-line",e.appendChild(t),n--}t.appendChild(e)}for(;n<0;)t.removeChild(t.lastChild),n++},append_line_numbers:function(e){const t=document.getElementById(e);if(null==t)return console.warn("[tln.js] Couldn't find textarea of id '"+e+"'");if(-1!=t.className.indexOf("tln-active"))return console.warn("[tln.js] textarea of id '"+e+"' is already numbered");t.classList.add("tln-active"),t.style={};const n=document.createElement("div");n.className="tln-wrapper",t.parentNode.insertBefore(n,t),TLN.update_line_numbers(t,n),TLN.eventList[e]=[];const l=["propertychange","input","keydown","keyup"],o=function(e,t){return function(n){(10!=+e.scrollLeft||37!=n.keyCode&&37!=n.which&&"ArrowLeft"!=n.code&&"ArrowLeft"!=n.key)&&36!=n.keyCode&&36!=n.which&&"Home"!=n.code&&"Home"!=n.key&&13!=n.keyCode&&13!=n.which&&"Enter"!=n.code&&"Enter"!=n.key&&"NumpadEnter"!=n.code||(e.scrollLeft=0),TLN.update_line_numbers(e,t)}}(t,n);for(let n=l.length-1;n>=0;n--)t.addEventListener(l[n],o),TLN.eventList[e].push({evt:l[n],hdlr:o});const r=["change","mousewheel","scroll"],s=function(e,t){return function(){t.scrollTop=e.scrollTop}}(t,n);for(let n=r.length-1;n>=0;n--)t.addEventListener(r[n],s),TLN.eventList[e].push({evt:r[n],hdlr:s})},remove_line_numbers:function(e){const t=document.getElementById(e);if(null==t)return console.warn("[tln.js] Couldn't find textarea of id '"+e+"'");if(-1==t.className.indexOf("tln-active"))return console.warn("[tln.js] textarea of id '"+e+"' isn't numbered");t.classList.remove("tln-active");const n=t.previousSibling;if("tln-wrapper"==n.className&&n.remove(),TLN.eventList[e]){for(let n=TLN.eventList[e].length-1;n>=0;n--){const l=TLN.eventList[e][n];t.removeEventListener(l.evt,l.hdlr)}delete TLN.eventList[e]}}};


//Nav and footer 
const setupNav = () => {
  var navLinks = [
    {name: "Status", url: "/"},
    {name: "Displays", url: "/displays"},
    {name: "Network", url: "/network"},
    {name: "Analysis", url: "/analysis"},
    {name: "Logging", url: "/logs"},
  ]
          
  //Build the NAV
  ulObj = el('ul');
  
  Object.keys(navLinks).forEach((k, i) => {
    liObj = el('li');
    if (window.location.pathname == navLinks[k].url) {
      addClass(liObj, "active");
    }
    aObj = el('a');
    aObj.innerText = navLinks[k].name;
    aObj.href = navLinks[k].url;
    liObj.appendChild(aObj);
    ulObj.appendChild(liObj);
  });
  
  navObj = el('nav');
  navObj.appendChild(ulObj);
  
  (document.getElementsByTagName('body')[0]).insertBefore(navObj, document.getElementsByTagName('main')[0]);
  
  //Build the Footer
  footerObj = el('footer');
  footerObj.innerHTML = "<span>\
      Github: <a href=\"https://github.com/joshwardell/CANserver\">https://github.com/joshwardell/CANserver</a>\
    </span>\
    <span>\
      <label for=\"theme\">Visual theme:</label>\
      <select name=\"theme\" id=\"theme\">\
        <option value=\"business\">Business</option>\
        <option value=\"pastel\">Pastel</option>\
      </select> \
    </span>";
  (document.getElementsByTagName('body')[0]).appendChild(footerObj);
}

document.addEventListener('DOMContentLoaded', setupNav);
    
    
    
// layout theme selector
const colorThemes = {
  // https://coolors.co/9a8f97-c3baba-e9e3e6-b2b2b2-736f72
  business: {
    '--color-1': 'hsla(315, 2%, 44%, 1)', /* sonic-silver */
    '--color-2': 'hsla(316, 5%, 58%, 1)', /* heliotrope-gray */
    '--color-3': 'hsla(0, 0%, 70%, 1)', /* silver-chalice */
    '--color-4': 'hsla(0, 7%, 75%, 1)', /* pale-silver */
    '--color-5': 'hsla(330, 12%, 90%, 1)' /* platinum */
  },
  // https://coolors.co/252323-70798c-f5f1ed-dad2bc-a99985
  pastel: {
    '--color-1': 'hsla(0, 3%, 14%, 1)',
    '--color-2': 'hsla(221, 11%, 49%, 1)',
    '--color-3': 'hsla(33, 17%, 59%, 1)',
    '--color-4': 'hsla(44, 29%, 80%, 1)',
    '--color-5': 'hsla(30, 29%, 95%, 1)'
  }
}

const activateColorTheme = name => {
  const colorTheme = colorThemes[name];

  if (colorTheme) {
    Object.keys(colorTheme).forEach(color => {
      document.documentElement.style.setProperty(color, colorTheme[color]);
    });
  }
}
    
const themeSelectEl = document.getElementById('theme');

if (themeSelectEl) {
    // activate persisted choice
    const persistedSelect = localStorage.getItem('theme') || 'business';
    activateColorTheme(persistedSelect);
    themeSelectEl.value = persistedSelect;

    // activate and persist new theme choice
    themeSelectEl.onchange = e => {
        const name = e.target.value;
        activateColorTheme(name);
        localStorage.setItem('theme', name);
    };
}


//Data fetching utils
const getText = url => fetch(url).then(r => r.text()).catch(console.error);
const getJSON = url => fetch(url).then(r => r.json()).catch(console.error);
const postData = (url, data) => fetch(url, { method: 'POST', body: data });

// dom utils
const el = (type, initial = {}) => {
    const element = document.createElement(type);

    if (initial.id) {
      element.id = initial.id;
    }

    if (initial.inner) {
        element.innerText = initial.inner;
    }

    if (initial.children) {
        initial.children.forEach(child => element.appendChild(child));
    }

    if (initial.className) {
        element.className = initial.className;
    }

    if (initial.attributes) {
        Object.keys(initial.attributes).forEach(attr => {
            element.setAttribute(attr, initial.attributes[attr]);
        })
    }

    return element;
}

const addClass = (element, classname) => element && element.classList && element.classList.add(classname);
const removeClass = (element, classname) => element && element.classList && element.classList.remove(classname);

const show = element => removeClass(element, 'hidden');
const hide = element => addClass(element, 'hidden');

const good = statusEl => {
    statusEl.innerText = 'Good';
    statusEl.classList.add('good');
    statusEl.classList.remove('bad');
};

const bad = statusEl => {
    statusEl.innerText = 'Bad';
    statusEl.classList.add('bad');
    statusEl.classList.remove('good');
};

const keyEventWithGracefulTabs = (textarea, e) => {
  const keyCode = e.keyCode || e.which;

  if (keyCode == 9) {
    e.preventDefault();

    var start = textarea.selectionStart;
    var end = textarea.selectionEnd;

    // set textarea value to: text before caret + tab + text after caret
    textarea.value = (textarea.value.substring(0, start)
        + "\t"
        + textarea.value.substring(end));

    // put caret at right position again
    textarea.selectionStart = textarea.selectionEnd = start + 1;
  }
}

// data conversion
const hexToDec = hex => {
  let result = 0;
  hex = hex.toLowerCase();

  for (var i = 0; i < hex.length; i++) {
    const digitValue = '0123456789abcdefgh'.indexOf(hex[i]);
    result = result * 16 + digitValue;
  }

  return result;
}

const decToHex = (dec, padding) => {
  var hex = Number(dec).toString(16).toUpperCase();
  padding = typeof (padding) === "undefined" || padding === null ? padding = 2 : padding;

  while (hex.length < padding) {
    hex = "0" + hex;
  }

  return hex;
}