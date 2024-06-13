/**
 * @file qtree.cpp
 * @description student implementation of QTree class used for storing image data
 *              CPSC 221 PA3
 *
 *              SUBMIT THIS FILE
 */

#include "qtree.h"

/**
 * Constructor that builds a QTree out of the given PNG.
 * Every leaf in the tree corresponds to a pixel in the PNG.
 * Every non-leaf node corresponds to a rectangle of pixels
 * in the original PNG, represented by an (x,y) pair for the
 * upper left corner of the rectangle and an (x,y) pair for
 * lower right corner of the rectangle. In addition, the Node
 * stores a pixel representing the average color over the
 * rectangle.
 *
 * The average color for each node in your implementation MUST
 * be determined in constant time. HINT: this will lead to nodes
 * at shallower levels of the tree to accumulate some error in their
 * average color value, but we will accept this consequence in
 * exchange for faster tree construction.
 * Note that we will be looking for specific color values in our
 * autograder, so if you instead perform a slow but accurate
 * average color computation, you will likely fail the test cases!
 *
 * Every node's children correspond to a partition of the
 * node's rectangle into (up to) four smaller rectangles. The node's
 * rectangle is split evenly (or as close to evenly as possible)
 * along both horizontal and vertical axes. If an even split along
 * the vertical axis is not possible, the extra line will be included
 * in the left side; If an even split along the horizontal axis is not
 * possible, the extra line will be included in the upper side.
 * If a single-pixel-wide rectangle needs to be split, the NE and SE children
 * will be null; likewise if a single-pixel-tall rectangle needs to be split,
 * the SW and SE children will be null.
 *
 * In this way, each of the children's rectangles together will have coordinates
 * that when combined, completely cover the original rectangle's image
 * region and do not overlap.
 */
QTree::QTree(const PNG& imIn) {
    // Initial dimensions of the image
    width = imIn.width();
    height = imIn.height();

    // The entire image is represented by the root node
    // Upper-left corner is (0, 0)
    // Lower-right corner is (width - 1, height - 1)
    root = BuildNode(imIn, make_pair(0, 0), make_pair(width - 1, height - 1));
}


/**
 * Overloaded assignment operator for QTrees.
 * Part of the Big Three that we must define because the class
 * allocates dynamic memory. This depends on your implementation
 * of the copy and clear funtions.
 *
 * @param rhs The right hand side of the assignment statement.
 */
QTree& QTree::operator=(const QTree& rhs) {
    // Step 1: Self-assignment check
    if (this == &rhs) {
        return *this;
    }

    // Step 2: Clear current contents
    Clear();

    // Step 3: Copy contents from rhs
    Copy(rhs);

    // Step 4: Return *this
    return *this;
}

/**
 * Render returns a PNG image consisting of the pixels
 * stored in the tree. may be used on pruned trees. Draws
 * every leaf node's rectangle onto a PNG canvas using the
 * average color stored in the node.
 *
 * For up-scaled images, no color interpolation will be done;
 * each rectangle is fully rendered into a larger rectangular region.
 *
 * @param scale multiplier for each horizontal/vertical dimension
 * @pre scale > 0
 */
PNG QTree::Render(unsigned int scale) const {
    // Create a scaled PNG canvas
    PNG canvas(width * scale, height * scale);

    // Start rendering from the root
    RenderNode(root, scale, canvas);

    return canvas;
}


void QTree::RenderNode(Node* node, unsigned int scale, PNG& canvas) const {
    if (node == nullptr) {
        // Base case: if the node is null, there's nothing to render.
        return;
    }

    if (IsLeaf(node)) {
        // If the node is a leaf, draw the rectangle it represents.
        // The rectangle's top-left corner is scaled up by 'scale' from the node's 'upLeft'.
        // The rectangle's bottom-right corner is 'lowRight', also scaled.
        for (unsigned int x = node->upLeft.first * scale; x < (node->lowRight.first + 1) * scale; x++) {
            for (unsigned int y = node->upLeft.second * scale; y < (node->lowRight.second + 1) * scale; y++) {
                // Ensure we don't go out of the canvas bounds.
                if (x < canvas.width() && y < canvas.height()) {
                    RGBAPixel* pixel = canvas.getPixel(x, y);
                    *pixel = node->avg;
                }
            }
        }
    } else {
        // Recursively render the children nodes.
        // The children nodes are responsible for drawing their respective quadrants.
        RenderNode(node->NW, scale, canvas);
        RenderNode(node->NE, scale, canvas);
        RenderNode(node->SW, scale, canvas);
        RenderNode(node->SE, scale, canvas);
    }
}


/**
 *  Prune function trims subtrees as high as possible in the tree.
 *  A subtree is pruned (cleared) if all of the subtree's leaves are within
 *  tolerance of the average color stored in the root of the subtree.
 *  NOTE - you may use the distanceTo function found in RGBAPixel.h
 *  Pruning criteria should be evaluated on the original tree, not
 *  on any pruned subtree. (we only expect that trees would be pruned once.)
 *
 * You may want a recursive helper function for this one.
 *
 * @param tolerance maximum RGBA distance to qualify for pruning
 * @pre this tree has not previously been pruned, nor is copied from a previously pruned tree.
 */
void QTree::Prune(double tolerance) {
    // Start pruning from the root
    PruneNode(root, tolerance);
}

void QTree::PruneNode(Node*& node, double tolerance) {
    if (node == nullptr) {
        return; // If the node is null, there's nothing to prune
    }

    // If the node is a leaf, there's no need to prune further
    if (IsLeaf(node)) {
        return;
    }

    // Recursively attempt to prune child nodes first
    PruneNode(node->NW, tolerance);
    PruneNode(node->NE, tolerance);
    PruneNode(node->SW, tolerance);
    PruneNode(node->SE, tolerance);

    // Check if we can prune this node after attempting to prune its children
    if (CanPrune(node, node->avg, tolerance)) {
        // Prune the children nodes
        ClearSubtree(node->NW);
        ClearSubtree(node->NE);
        ClearSubtree(node->SW);
        ClearSubtree(node->SE);
    }
}

bool QTree::CanPrune(Node* node, RGBAPixel avgColor, double tolerance) const {
    if (node == nullptr) {
        return true; // A null node is considered prunable
    }

    if (IsLeaf(node)) {
        // Check color distance for leaf nodes
        return node->avg.distanceTo(avgColor) <= tolerance;
    }

    // Check if all children are prunable
    return CanPrune(node->NW, avgColor, tolerance) &&
           CanPrune(node->NE, avgColor, tolerance) &&
           CanPrune(node->SW, avgColor, tolerance) &&
           CanPrune(node->SE, avgColor, tolerance);
}

bool QTree::IsLeaf(Node* node) const {
    return node != nullptr &&
           node->NW == nullptr && node->NE == nullptr &&
           node->SW == nullptr && node->SE == nullptr;
}

void QTree::ClearSubtree(Node*& node) {
    if (node == nullptr) {
        return; // No need to clear if the node is already null
    }

    // Recursively clear children
    ClearSubtree(node->NW);
    ClearSubtree(node->NE);
    ClearSubtree(node->SW);
    ClearSubtree(node->SE);

    // Delete the node and set it to null to prevent access to deleted memory
    delete node;
    node = nullptr;
}



/**
 *  FlipHorizontal rearranges the contents of the tree, so that
 *  its rendered image will appear mirrored across a vertical axis.
 *  This may be called on a previously pruned/flipped/rotated tree.
 *
 *  After flipping, the NW/NE/SW/SE pointers must map to what will be
 *  physically rendered in the respective NW/NE/SW/SE corners, but it
 *  is no longer necessary to ensure that 1-pixel wide rectangles have
 *  null eastern children
 *  (i.e. after flipping, a node's NW and SW pointers may be null, but
 *  have non-null NE and SE)
 * 
 *  You may want a recursive helper function for this one.
 */
void QTree::FlipHorizontal() {
    FlipNodeHorizontal(root);
}

void QTree::FlipNodeHorizontal(Node* node) {
    if (node == nullptr) {
        return; // Base case: if the node is null, there's nothing to flip
    }

    // Swap the child nodes horizontally
    std::swap(node->NW, node->NE);
    std::swap(node->SW, node->SE);

    // Update the coordinates of the children to reflect the horizontal flip
    UpdateCoordinatesAfterFlip(node->NW);
    UpdateCoordinatesAfterFlip(node->NE);
    UpdateCoordinatesAfterFlip(node->SW);
    UpdateCoordinatesAfterFlip(node->SE);

    // Recursively flip the child subtrees
    FlipNodeHorizontal(node->NW);
    FlipNodeHorizontal(node->NE);
    FlipNodeHorizontal(node->SW);
    FlipNodeHorizontal(node->SE);
}

void QTree::UpdateCoordinatesAfterFlip(Node* node) {
    if (node == nullptr) {
        return;
    }

    unsigned int totalWidth = width - 1;
    unsigned int newLeftX = totalWidth - node->lowRight.first;
    unsigned int newRightX = totalWidth - node->upLeft.first;

    node->upLeft.first = newLeftX;
    node->lowRight.first = newRightX;
}




/**
 *  RotateCCW rearranges the contents of the tree, so that its
 *  rendered image will appear rotated by 90 degrees counter-clockwise.
 *  This may be called on a previously pruned/flipped/rotated tree.
 *
 *  Note that this may alter the dimensions of the rendered image, relative
 *  to its original dimensions.
 *
 *  After rotation, the NW/NE/SW/SE pointers must map to what will be
 *  physically rendered in the respective NW/NE/SW/SE corners, but it
 *  is no longer necessary to ensure that 1-pixel tall or wide rectangles
 *  have null eastern or southern children
 *  (i.e. after rotation, a node's NW and NE pointers may be null, but have
 *  non-null SW and SE, or it may have null NW/SW but non-null NE/SE)
 *
 *  You may want a recursive helper function for this one.
 */
void QTree::RotateCCW() {
    // Swap the dimensions of the entire image
    std::swap(width, height);
    RotateNodeCCW(root, {0, 0}, {width - 1, height - 1});
}

void QTree::RotateNodeCCW(Node* node, pair<unsigned int, unsigned int> ul, pair<unsigned int, unsigned int> lr) {
    if (node == nullptr) {
        return; // Base case: if the node is null, there's nothing to rotate
    }

    // Rotate the node's children counterclockwise
    Node* temp = node->NW;
    node->NW = node->SW;
    node->SW = node->SE;
    node->SE = node->NE;
    node->NE = temp;

    // Calculate new boundaries after rotation
    unsigned int midX = (ul.first + lr.first) / 2;
    unsigned int midY = (ul.second + lr.second) / 2;

    // Update the coordinates of the children to reflect the new layout
    if (node->NW) RotateNodeCCW(node->NW, {ul.second, lr.first - midX}, {midY, lr.first});
    if (node->NE) RotateNodeCCW(node->NE, {ul.second, ul.first}, {midY, lr.first - midX});
    if (node->SW) RotateNodeCCW(node->SW, {midY + 1, lr.first - midX}, {lr.second, lr.first});
    if (node->SE) RotateNodeCCW(node->SE, {midY + 1, ul.first}, {lr.second, lr.first - midX});

    // Update the node's coordinates
    node->upLeft = {ul.second, lr.first - (lr.first - ul.first)};
    node->lowRight = {lr.second, lr.first - (ul.first)};
}





/**
 * Destroys all dynamically allocated memory associated with the
 * current QTree object. Complete for PA3.
 * You may want a recursive helper function for this one.
 */
void QTree::Clear() {
    // Clear the tree starting from the root
    ClearNode(root);

    // Reset tree attributes
    root = nullptr;
    width = 0;
    height = 0;
}

// Recursive helper function for clearing the tree
void QTree::ClearNode(Node* node) {
    if (node == nullptr) {
        return;
    }

    // Recursively clear children
    ClearNode(node->NW);
    ClearNode(node->NE);
    ClearNode(node->SW);
    ClearNode(node->SE);

    // Delete the current node
    delete node;
}


/**
 * Copies the parameter other QTree into the current QTree.
 * Does not free any memory. Called by copy constructor and operator=.
 * You may want a recursive helper function for this one.
 * @param other The QTree to be copied.
 */
void QTree::Copy(const QTree& other) {
    // Copy primitive attributes
    width = other.width;
    height = other.height;

    // Deep copy the tree structure
    root = CopyNode(other.root);
}

// Recursive helper function to copy nodes
Node* QTree::CopyNode(Node* otherNode) {
    if (otherNode == nullptr) {
        return nullptr;
    }

    // Create a new node with the same data as otherNode
    Node* newNode = new Node(otherNode->upLeft, otherNode->lowRight, otherNode->avg);

    // Recursively copy children
    newNode->NW = CopyNode(otherNode->NW);
    newNode->NE = CopyNode(otherNode->NE);
    newNode->SW = CopyNode(otherNode->SW);
    newNode->SE = CopyNode(otherNode->SE);

    return newNode;
}


/**
 * Private helper function for the constructor. Recursively builds
 * the tree according to the specification of the constructor.
 * @param img reference to the original input image.
 * @param ul upper left point of current node's rectangle.
 * @param lr lower right point of current node's rectangle.
 */
Node* QTree::BuildNode(const PNG& img, pair<unsigned int, unsigned int> ul, pair<unsigned int, unsigned int> lr) {
    // Base case: single pixel region
    if (ul == lr) {
        RGBAPixel* pixel = img.getPixel(ul.first, ul.second);
        return new Node(ul, lr, *pixel);
    }

    // Calculate midpoints for splitting
    unsigned int midX = (ul.first + lr.first) / 2;
    unsigned int midY = (ul.second + lr.second) / 2;

    // Initialize child nodes
    Node *NW, *NE, *SW, *SE;
    NW = NE = SW = SE = nullptr;

    // Recursively create child nodes if they are within bounds
    if (ul.first <= midX && ul.second <= midY) // Check if NW child should exist
        NW = BuildNode(img, ul, {midX, midY});

    if (midX + 1 <= lr.first && ul.second <= midY) // Check if NE child should exist
        NE = BuildNode(img, {midX + 1, ul.second}, {lr.first, midY});

    if (ul.first <= midX && midY + 1 <= lr.second) // Check if SW child should exist
        SW = BuildNode(img, {ul.first, midY + 1}, {midX, lr.second});

    if (midX + 1 <= lr.first && midY + 1 <= lr.second) // Check if SE child should exist
        SE = BuildNode(img, {midX + 1, midY + 1}, lr);

    // Create the current node with averaged color from children
    RGBAPixel avgColor = CalculateAverageColor(NW, NE, SW, SE);
    Node* node = new Node(ul, lr, avgColor);
    node->NW = NW;
    node->NE = NE;
    node->SW = SW;
    node->SE = SE;

    return node;
}

RGBAPixel QTree::CalculateAverageColor(Node* NW, Node* NE, Node* SW, Node* SE) {
    // Initialize color sums and pixel count
    unsigned int sumRed = 0, sumGreen = 0, sumBlue = 0, pixelCount = 0;

    // Helper function to add color values from a node if it is not null
    auto addColor = [&](Node* node) {
        if (node) {
            unsigned int nodePixelCount = (node->lowRight.first - node->upLeft.first + 1) * 
                                          (node->lowRight.second - node->upLeft.second + 1);
            sumRed += node->avg.r * nodePixelCount;
            sumGreen += node->avg.g * nodePixelCount;
            sumBlue += node->avg.b * nodePixelCount;
            pixelCount += nodePixelCount;
        }
    };

    // Add color values from each child node that is not null
    addColor(NW);
    addColor(NE);
    addColor(SW);
    addColor(SE);

    // Calculate the average color
    RGBAPixel avgColor;
    if (pixelCount > 0) {
        avgColor.r = sumRed / pixelCount;
        avgColor.g = sumGreen / pixelCount;
        avgColor.b = sumBlue / pixelCount;
    } else {
        // If there are no children, this means we're at a leaf or pruned node.
        // The average color is just the node's color. For this, we arbitrarily pick NW.
        // This will only occur if all children are nullptr, which should be handled by the caller.
        avgColor = NW ? NW->avg : RGBAPixel(); // Assuming RGBAPixel() constructs a default color
    }

    return avgColor;
}





/*********************************************************/
/*** IMPLEMENT YOUR OWN PRIVATE MEMBER FUNCTIONS BELOW ***/
/*********************************************************/

