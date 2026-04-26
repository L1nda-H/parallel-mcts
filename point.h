#include <string>

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

	Point (std::string coord) {
		int col =  static_cast<int>(coord[0]);
		if (col >= 'I') col --;
		// +1 to account for board padding
		col -= 65 + 1;
		i = col;
		j = std::stoi(coord.substr(1));
	}

	static std::string pt_to_gtp(Point p) {
		std::string coord = "";
		coord += (static_cast<char>(p.i + 64));
		coord += (std::to_string(p.j));
		return coord;
	}
};

#endif