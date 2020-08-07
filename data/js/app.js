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
  
  
  //Setup themes
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

//Based off of https://github.com/wjbryant/taboverride
const keyEventWithGracefulTabs = (target, e) => {
  const keyCode = e.keyCode || e.which;
  const tab = "\t";
  const tabLen = 1;
  
  if (keyCode == 9) {
    e.preventDefault();

    var selStart = target.selectionStart;
    var selEnd = target.selectionEnd;
    var text = target.value;
    var sel = text.slice(selStart, selEnd);
    var initScrollTop = target.scrollTop;

    if (selStart !== selEnd && sel.indexOf('\n') !== -1) {
      // for multiple lines, only insert / remove tabs from the beginning of each line
      // find the start of the first selected line
      if (selStart === 0 || text.charAt(selStart - 1) === '\n') {
          // the selection starts at the beginning of a line
          startLine = selStart;
      } else {
          // the selection starts after the beginning of a line
          // set startLine to the beginning of the first partially selected line
          // subtract 1 from selStart in case the cursor is at the newline character,
          // for instance, if the very end of the previous line was selected
          // add 1 to get the next character after the newline
          // if there is none before the selection, lastIndexOf returns -1
          // when 1 is added to that it becomes 0 and the first character is used
          startLine = text.lastIndexOf('\n', selStart - 1) + 1;
      }
      
      // find the end of the last selected line
      if (selEnd === text.length || text.charAt(selEnd) === '\n') {
          // the selection ends at the end of a line
          endLine = selEnd;
      } else if (text.charAt(selEnd - 1) === '\n') {
          // the selection ends at the start of a line, but no
          // characters are selected - don't indent this line
          endLine = selEnd - 1;
      } else {
          // the selection ends before the end of a line
          // set endLine to the end of the last partially selected line
          endLine = text.indexOf('\n', selEnd);
          if (endLine === -1) {
              endLine = text.length;
          }
      }
      
      if (e.shiftKey == false) {
        numTabs = 1; // for the first tab

        // insert tabs at the beginning of each line of the selection
        target.value = text.slice(0, startLine) + tab +
            text.slice(startLine, endLine).replace(/\n/g, function () {
                numTabs += 1;
                return '\n' + tab;
            }) + text.slice(endLine);

        // set start and end points
        // the selection start is always moved by 1 character
        target.selectionStart = selStart + tabLen;
        // move the selection end over by the total number of tabs inserted
        target.selectionEnd = selEnd + (numTabs * tabLen);
        target.scrollTop = initScrollTop;
        
      } else if (e.shiftKey == true) {
        // if the untab key combo was pressed, remove tabs instead of inserting them
        numTabs = 0;
        
        if (text.slice(startLine).indexOf(tab) === 0) {
            // is this tab part of the selection?
            if (startLine === selStart) {
                // it is, remove it
                sel = sel.slice(tabLen);
            } else {
                // the tab comes before the selection
                preTab = tabLen;
            }
            startTab = tabLen;
        }
        
        target.value = text.slice(0, startLine) + text.slice(startLine + preTab, selStart) +
            sel.replace(new RegExp('\n' + tab, 'g'), function () {
                numTabs += 1;
                return '\n';
            }) + text.slice(selEnd);
        
        // set start and end points
        // set start first for Opera
        target.selectionStart = selStart - preTab; // preTab is 0 or tabLen
        // move the selection end over by the total number of tabs removed
        target.selectionEnd = selEnd - startTab - numTabs;
      }
    } else {
      // single line selection

      // tab key combo - insert a tab
      if (e.shiftKey == false) {
          target.value = text.slice(0, selStart) + tab + text.slice(selEnd);
          target.selectionEnd = target.selectionStart = selStart + tabLen;
          target.scrollTop = initScrollTop;
      } else if (e.shiftKey == true) {
          // if the untab key combo was pressed, remove a tab instead of inserting one

          // if the character before the selection is a tab, remove it
          if (text.slice(selStart - tabLen).indexOf(tab) === 0) {
              target.value = text.slice(0, selStart - tabLen) + text.slice(selStart);

              // set start and end points
              target.selectionEnd = target.selectionStart = selStart - tabLen;
              target.scrollTop = initScrollTop;
          }
      }
    }
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