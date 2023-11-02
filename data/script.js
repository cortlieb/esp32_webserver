window.addEventListener('load', getStates);

function toggleCheckbox(element) {
	var xhr = new XMLHttpRequest(); //TODO: Deklaration austauschen mit let
	if (element.checked) {
		xhr.open("GET", "/update?output=" + element.id + "&state=1", true);
		document.getElementById(element.id + "_state").innerHTML = "ON";
	}
	else {
		xhr.open("GET", "/update?output=" + element.id + "&state=0", true);
		document.getElementById(element.id + "_state").innerHTML = "OFF";
	}
	xhr.send();
}

function getStates() {
	var xhr = new XMLHttpRequest(); //TODO: Deklaration austauschen mit let
	xhr.onreadystatechange = function () {
		if (this.readyState == 4 && this.status == 200) {
			var stateObj = JSON.parse(this.responseText);
			console.log(stateObj);
			for (i in stateObj.outputs) {
				var output = stateObj.outputs[i].output;
				var state = stateObj.outputs[i].state;
				console.log(output);
				console.log(state);
				if (state == "1") {
					document.getElementById(output).checked = true;
					document.getElementById(output + "_state").innerHTML = "ON";
				} else {
					document.getElementById(output).checked = false;
					document.getElementById(output + "_state").innerHTML = "OFF";
				}
			}
		}
	}
	xhr.open("GET", "/states", true);
	xhr.send();
}