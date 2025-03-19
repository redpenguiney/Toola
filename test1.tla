function void(){}();

// comment
i32 j = -2

bool jk = 3 == j

class A {
	f64 a
	string b;
	string& cc

	var method1 = function void(A& self, i32 b) {}

	void(A&, i32) method2 = function void(A& self, i32 c) {
	
	
		c *= 2

	}
}

var aConstructor = function A() {
	//A obj = new A()
	//obj.a = 3
	//return obj
}
A aobject = aConstructor();


//i32: 32 bit signed int, passed by value
//i32&: reference to int, so the int is passed by reference
//

var joe = function i32(A& obj, string str) {
	if (true) {
	}
	else {
	
	}	
	;
	while (true) {
	
	}

	for (i32 die = 0, false, die += 1) {
	
	}

	//for (i32 dier = 1, dier > -4, dier -= 1) {
	
	//}

	//obj.method1(obj, 31)

	//int a = (3 +obj.b[3.073] - 124 %= 3)
	i32 a = 0
	a *= 2 - 3;

	return 5
}


f64 a = 1;
f64 b = 2;
f64& aref = a;
f64& aref2 = b;
aref = b; // ok: aref now refers to b
aref += aref; // ok: b now == 4
//aref = 3.0; // bad: can only refer to an rvalue
//aref = a + b; // bad: can only refer to an rvalue

a *= 3;
a *= 4.0;

i32 c = 4;
// c += b; // bad: c + b is a double and cannot be assigned into an int

//string jee = "die alone"

//i32 p = joe()

