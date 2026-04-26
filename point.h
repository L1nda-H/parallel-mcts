#include <string>
#include <cctype>
#include <stdexcept>

#ifndef POINT_H
#define POINT_H  

class Point{
public:
	int i,j;
	Point(int a, int b):i(a),j(b){}

	Point():i(0),j(0){}
	
	Point(const Point& p){
		i = p.i;
		j = p.j;
	}

	Point(std::string coord, int bsize) {
		if (coord == "pass" || coord == "PASS") {
			i = -1;
			j = -1;
			return;
		}

		char letter = std::toupper(coord[0]);
		if (letter >= 'I') letter--;
		i = letter - 'A' + 1;

		try {
			j = (bsize + 1) - std::stoi(coord.substr(1));
		} catch (const std::invalid_argument& e) {
			i = -1; j = -1; 
		}
	}

	static std::string pt_to_gtp(Point p, int bsize) {
		if (p.i == -1 || p.j == -1) {
			return "pass";
		}

		std::string coord = "";
		
		char col = (p.i - 1) + 'A';
		if (col >= 'I') col++;
		
		coord += col;
		coord += std::to_string((bsize + 1) - p.j);
		
		return coord;
	}
};

#endif