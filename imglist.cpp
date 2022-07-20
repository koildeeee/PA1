// File:        imglist.cpp
// Date:        2022-01-27 10:21
// Description: Contains partial implementation of ImgList class
//              for CPSC 221 2021W2 PA1
//              Function bodies to be completed by yourselves
//
// ADD YOUR FUNCTION IMPLEMENTATIONS IN THIS FILE
//

#include "imglist.h"

#include <math.h> // provides fmax, fmin, and fabs functions

/**************************
* MISCELLANEOUS FUNCTIONS *
**************************/

/*
* This function is NOT part of the ImgList class,
* but will be useful for one of the ImgList functions.
* Returns the "difference" between two hue values.
* PRE: hue1 is a double between [0,360).
* PRE: hue2 is a double between [0,360).
* 
* The hue difference is the absolute difference between two hues,
* but takes into account differences spanning the 360 value.
* e.g. Two pixels with hues 90 and 110 differ by 20, but
*      two pixels with hues 5 and 355 differ by 10.
*/
double HueDiff(double hue1, double hue2) {
  return fmin(fabs(hue1 - hue2), fabs(360 + fmin(hue1, hue2) - fmax(hue1, hue2)));
}

/*********************
* CONSTRUCTORS, ETC. *
*********************/

/*
* Default constructor. Makes an empty list
*/
ImgList::ImgList() {
  northwest = NULL;
  southeast = NULL;
}

/*
* Creates a list from image data
* PRE: img has dimensions of at least 1x1
*/
ImgList::ImgList(PNG& img) {
  northwest = new ImgNode();
  northwest->colour = *img.getPixel(0,0);
  CreateFirstRow(northwest, 0, img.width(), img);
  ImgNode* prev = northwest;
  ImgNode* lastNode;
  for (unsigned y = 1; y < img.height(); y++) {
    ImgNode* curr = new ImgNode();
    curr->colour = *img.getPixel(0, y);
    prev->south = curr;
    curr->north = prev;
    lastNode = CreateRow(curr, y, img.width(), img);
    prev = curr;
  }
  southeast = lastNode;
}

/*
* Copy constructor.
* Creates this this to become a separate copy of the data in otherlist
*/
ImgList::ImgList(const ImgList& otherlist) {
  // build the linked node structure using otherlist as a template
  Copy(otherlist);
}

/*
* Assignment operator. Enables statements such as list1 = list2;
*   where list1 and list2 are both variables of ImgList type.
* POST: the contents of this list will be a physically separate copy of rhs
*/
ImgList& ImgList::operator=(const ImgList& rhs) {
  // Re-build any existing structure using rhs as a template
  
  if (this != &rhs) { // if this list and rhs are different lists in memory
    // release all existing heap memory of this list
    Clear();    
    
    // and then rebuild this list using rhs as a template
    Copy(rhs);
  }
  
  return *this;
}

/*
* Destructor.
* Releases any heap memory associated with this list.
*/
ImgList::~ImgList() {
  // Ensure that any existing heap memory is deallocated
  Clear();
}

/************
* ACCESSORS *
************/

/*
* Returns the horizontal dimension of this list (counted in nodes)
* Note that every row will contain the same number of nodes, whether or not
*   the list has been carved.
* We expect your solution to take linear time in the number of nodes in the
*   x dimension.
*/
unsigned int ImgList::GetDimensionX() const {
  int result = 0;
  ImgNode* temp = northwest;
  while (temp != nullptr) {
    result++;
    temp = temp->east;
  }
  return result;
}

/*
* Returns the vertical dimension of the list (counted in nodes)
* It is useful to know/assume that the grid will never have nodes removed
*   from the first or last columns. The returned value will thus correspond
*   to the height of the PNG image from which this list was constructed.
* We expect your solution to take linear time in the number of nodes in the
*   y dimension.
*/
unsigned int ImgList::GetDimensionY() const {
  int result = 0;
  ImgNode* temp = northwest;
  while (temp != nullptr) {
    result++;
    temp = temp->south;
  }
  return result;
}

/*
* Returns the horizontal dimension of the list (counted in original pixels, pre-carving)
* The returned value will thus correspond to the width of the PNG image from
*   which this list was constructed.
* We expect your solution to take linear time in the number of nodes in the
*   x dimension.
*/
unsigned int ImgList::GetDimensionFullX() const {
  int result = 0;
  ImgNode* temp = northwest;
  while (temp != nullptr) {
    result++;
    result = result + temp->skipright;
    temp = temp->east;
  }
  return result;
}

/*
* Returns a pointer to the node which best satisfies the specified selection criteria.
* The first and last nodes in the row cannot be returned.
* PRE: rowstart points to a row with at least 3 physical nodes
* PRE: selectionmode is an integer in the range [0,1]
* PARAM: rowstart - pointer to the first node in a row
* PARAM: selectionmode - criterion used for choosing the node to return
*          0: minimum luminance across row, not including extreme left or right nodes
*          1: node with minimum total of "hue difference" with its left neighbour and with its right neighbour.
*        In the (likely) case of multiple candidates that best match the criterion,
*        the left-most node satisfying the criterion (excluding the row's starting node)
*        will be returned.
* A note about "hue difference": For PA1, consider the hue value to be a double in the range [0, 360).
* That is, a hue value of exactly 360 should be converted to 0.
* The hue difference is the absolute difference between two hues,
* but be careful about differences spanning the 360 value.
* e.g. Two pixels with hues 90 and 110 differ by 20, but
*      two pixels with hues 5 and 355 differ by 10.
*/
ImgNode* ImgList::SelectNode(ImgNode* rowstart, int selectionmode) {
  ImgNode* retNode = rowstart->east; // since first node can never be returned
  ImgNode* curr = rowstart->east;

  while (curr->east != NULL) { // not the last node in row, so we can continue comparing
    if (selectionmode == 0) {
      if ((double) retNode->colour.l > (double) curr->colour.l) {
        retNode = curr;
      }
    } else if (selectionmode == 1) { // otherwise ret stays the same
      double retLeftDiff = HueDiff(retNode->colour.h, retNode->west->colour.h);
      double retRightDiff = HueDiff(retNode->colour.h, retNode->east->colour.h);
      double currLeftDiff = HueDiff(curr->colour.h, curr->west->colour.h);
      double currRightDiff = HueDiff(curr->colour.h, curr->east->colour.h);
      if ((retLeftDiff + retRightDiff) > (currLeftDiff + currRightDiff)) {
        retNode = curr;
      }
    }
    curr = curr->east;
  }
  return retNode;
}

/*
* Renders this list's pixel data to a PNG, with or without filling gaps caused by carving.
* PRE: fillmode is an integer in the range of [0,2]
* PARAM: fillgaps - whether or not to fill gaps caused by carving
*          false: render one pixel per node, ignores fillmode
*          true: render the full width of the original image,
*                filling in missing nodes using fillmode
* PARAM: fillmode - specifies how to fill gaps
*          0: solid, uses the same colour as the node at the left of the gap
*          1: solid, using the averaged values (all channels) of the nodes at the left and right of the gap
*             Note that "average" for hue will use the closer of the angular distances,
*             e.g. average of 10 and 350 will be 0, instead of 180.
*             Average of diametric hue values will use the smaller of the two averages
*             e.g. average of 30 and 210 will be 120, and not 300
*                  average of 170 and 350 will be 80, and not 260
*          2: *** OPTIONAL - FOR BONUS ***
*             linear gradient between the colour (all channels) of the nodes at the left and right of the gap
*             e.g. a gap of width 1 will be coloured with 1/2 of the difference between the left and right nodes
*             a gap of width 2 will be coloured with 1/3 and 2/3 of the difference
*             a gap of width 3 will be coloured with 1/4, 2/4, 3/4 of the difference, etc.
*             Like fillmode 1, use the smaller difference interval for hue,
*             and the smaller-valued average for diametric hues
*/
PNG ImgList::Render(bool fillgaps, int fillmode) const {
  // Add/complete your implementation below
  PNG outpng; //this will be returned later. Might be a good idea to resize it at some point.
  unsigned int fullwidth = GetDimensionFullX();
  unsigned int width = GetDimensionX();
  unsigned int height = GetDimensionY();
  
  if (!fillgaps) {
    outpng.resize(width, height);
    RenderImage(northwest, width, height, outpng);
  } else {
    outpng.resize(fullwidth, height);
    ImgNode *curr = northwest; 
    ImgNode *currY = northwest;
    for (unsigned int y = 0; y < height; y++) {
      for (unsigned int x = 0; x < fullwidth; x++) {
        HSLAPixel *pixel = outpng.getPixel(x, y);
        int skip = 0;
        *pixel = curr->colour;
        if (curr->skipright > 0) {
          skip = curr->skipright;
          while (skip > 0) {
            x++;
            pixel = outpng.getPixel(x,y);
            if (fillmode == 0) {
              *pixel = curr->colour;
            } else if (fillmode == 1) {
              pixel->h = GetSmallerHue(curr->colour.h, curr->east->colour.h);
              pixel->s = (curr->colour.s + curr->east->colour.s) / 2;
              pixel->l = (curr->colour.l + curr->east->colour.l) / 2;
              pixel->a = (curr->colour.a + curr->east->colour.a) / 2;
            }
            skip--;
          }
        }
        if (curr->east != NULL) {
          curr = curr->east;
        } else {
          curr = currY;
        }
      }
      if (currY->south != NULL) {
        currY = currY->south;
        curr = currY;
      }
    }
  }

  return outpng;
}

/************
* MODIFIERS *
************/

/*
* Removes exactly one node from each row in this list, according to specified criteria.
* The first and last nodes in any row cannot be carved.
* PRE: this list has at least 3 nodes in each row
* PRE: selectionmode is an integer in the range [0,1]
* PARAM: selectionmode - see the documentation for the SelectNode function.
* POST: this list has had one node removed from each row. Neighbours of the created
*       gaps are linked appropriately, and their skip values are updated to reflect
*       the size of the gap.
*/
void ImgList::Carve(int selectionmode) {
  ImgNode* curr = northwest;
  for (unsigned y = 0; y < GetDimensionY(); y++) {
    ImgNode* toRemove = SelectNode(curr, selectionmode);
    ImgNode* n;
    ImgNode* s;
    ImgNode* e;
    ImgNode* w;
    e = toRemove->east;
    w = toRemove->west;
    IncSkipEW(toRemove, w, e);
    e->west = w;
    w->east = e;
    // assert(e->west->east == e);
    // assert(w->east->west == w);

    if (y == 0 || toRemove->north == nullptr) {
      s = toRemove->south;
      s->skipup = s->skipup + 1; // FIX THIS
      n = nullptr;
      // s->north = nullptr; // this makes it segfault!
    } else if (y == GetDimensionY() - 1 || toRemove->south == nullptr) {
      n = toRemove->north;
      n->skipdown = n->skipdown + 1; // FIX THIS
      s = nullptr;
      // n->south = nullptr; // this makes it segfault!
    } else {
      n = toRemove->north;
      s = toRemove->south;
      IncSkipNS(toRemove, n, s);
      n->south = s;
      s->north = n;
      // assert(n->south->north == n);
      // assert(s->north->south == s);
      // assert(n->south->east->north->west == n);
    }
    toRemove->north = NULL;
    toRemove->south = NULL;
    toRemove->east = NULL;
    toRemove->west = NULL;
    curr = curr->south;
    delete toRemove;
  }
}

// note that a node on the boundary will never be selected for removal
/*
* Removes "rounds" number of nodes (up to a maximum of node width - 2) from each row,
* based on specific selection criteria.
* Note that this should remove one node from every row, repeated "rounds" times,
* and NOT remove "rounds" nodes from one row before processing the next row.
* PRE: selectionmode is an integer in the range [0,1]
* PARAM: rounds - number of nodes to remove from each row
*        If rounds exceeds node width - 2, then remove only node width - 2 nodes from each row.
*        i.e. Ensure that the final list has at least two nodes in each row.
* POST: this list has had "rounds" nodes removed from each row. Neighbours of the created
*       gaps are linked appropriately, and their skip values are updated to reflect
*       the size of the gap.
*/
void ImgList::Carve(unsigned int rounds, int selectionmode) {
  // add your implementation here
  if (rounds >= (GetDimensionY() - 2)) {
    rounds = GetDimensionY() - 2;
  }

  while (rounds > 0) {
    Carve(selectionmode);
    rounds = rounds - 1;
  }
}


/*
* Helper function deallocates all heap memory associated with this list,
* puts this list into an "empty" state. Don't forget to set your member attributes!
* POST: this list has no currently allocated nor leaking heap memory,
*       member attributes have values consistent with an empty list.
*/
void ImgList::Clear() {
  ImgNode* curr = northwest;
  ImgNode* currY = northwest;
  unsigned int width = GetDimensionX();
  unsigned int height = GetDimensionY();

  // int x = 0; //for testing
  // int y = 0; //for testing
  // while (currY != nullptr) {
  //   ImgNode* tempY = currY->south;
  //   while (curr != nullptr) {
  //     cout << "freeing x = " << x << ", y = " << y << endl;
  //     ImgNode* temp = curr;
  //     curr = curr->east;
  //     delete temp;
  //     x++;
  //   }
  //   currY = tempY;
  //   curr = currY;
  //   x = 0;
  //   y++;
  // }
  // northwest = NULL;
  // southeast = NULL;

  for (unsigned y = 0; y < height; y++) {
    for (unsigned x = 0; x < width; x++) {
      cout << "deleting node x = " << x << ", y = " << y << endl;
      ImgNode* temp = curr;
      curr = curr->east;
      delete temp;
    }
    currY = currY->south;
    curr = currY;
  }
  northwest = NULL;
  southeast = NULL;
}

/* ************************
*  * OPTIONAL - FOR BONUS *
** ************************
* Helper function copies the contents of otherlist and sets this list's attributes appropriately
* PRE: this list is empty
* PARAM: otherlist - list whose contents will be copied
* POST: this list has contents copied from by physically separate from otherlist
*/
void ImgList::Copy(const ImgList& otherlist) {
  // add your implementation here
  
}

/*************************************************************************************************
* IF YOU DEFINED YOUR OWN PRIVATE FUNCTIONS IN imglist.h, YOU MAY ADD YOUR IMPLEMENTATIONS BELOW *
*************************************************************************************************/

void ImgList::CreateFirstRow(ImgNode *curr, int rowNum, unsigned width, PNG &img) {
  for (unsigned x = 1; x < width; x++) { // starts at 1 bc we already have curr node
    ImgNode *newNode = new ImgNode();
    newNode->colour = *img.getPixel(x, rowNum);
    curr->east = newNode;
    newNode->west = curr;
    // assert(newNode->west->east == newNode);
    curr = newNode;
  }
}

ImgNode* ImgList::CreateRow(ImgNode *curr, int rowNum, unsigned width, PNG &img) {
  for (unsigned x = 1; x < width; x++) { // starts at 1 bc we already have curr node
    ImgNode *newNode = new ImgNode();
    newNode->colour = *img.getPixel(x, rowNum);
    curr->east = newNode;
    newNode->west = curr;
    curr->north->east->south = newNode;
    newNode->north = curr->north->east;
    // assert(newNode->west->east == newNode);
    // assert(newNode->west->north->east->south == newNode);
    curr = newNode;
  }
  return curr;
}

void ImgList::RenderImage(ImgNode *init, unsigned width, unsigned height, PNG& outpng) const {
  ImgNode *curr = init;
  ImgNode *currY = init;

  for (unsigned y = 0; y < height; y++) {
    for (unsigned x = 0; x < width; x++) {
      HSLAPixel *pixel = outpng.getPixel(x, y);
      *pixel = curr->colour;
      // assert(*pixel == curr->colour);
      if (curr->east != NULL) {
        curr = curr->east;
      } else {
        curr = currY; 
      }
    } if (currY->south != NULL) {
      currY = currY->south;
      curr = currY;
    }
  }
}

double ImgList::GetSmallerHue(double hue1, double hue2) const {
  double stdAvg = (hue1 + hue2) / 2;
  double diametricAvg = abs(180 - stdAvg);
  
  if ((hue1 >= 180 && hue2 >= 180) || (hue1 < 180 && hue2 < 180)) {
    return stdAvg;
  }

  if (stdAvg < diametricAvg) {
    return stdAvg;
  } else {
    return diametricAvg;
  }
}

int ImgList::Max(int a, int b) {
  if (a > b) {
    return a;
  } else {
    return b;
  }
}

void ImgList::IncSkipEW(ImgNode* toRemove, ImgNode* w, ImgNode* e) {
  // check if null
  if (toRemove->skipleft == toRemove->skipright) {
    w->skipright = w->skipright + toRemove->skipleft + 1;
    e->skipleft = e->skipleft + toRemove->skipright + 1;
  } else {
    int newSkip = Max(w->skipright + toRemove->skipleft, e->skipleft + toRemove->skipright);
    w->skipright = newSkip;
    e->skipleft = newSkip;
  }
}

void ImgList::IncSkipNS(ImgNode* toRemove, ImgNode* n, ImgNode* s) {
  // check if null
  if (toRemove->skipup == toRemove->skipdown) {
    n->skipup = n->skipup + toRemove->skipdown + 1;
    s->skipdown = s->skipdown + toRemove->skipup + 1;
  } else {
    int newSkip = Max(n->skipdown + toRemove->skipup, s->skipup + toRemove->skipdown);
    n->skipdown = newSkip;
    s->skipup = newSkip;
  }
}

