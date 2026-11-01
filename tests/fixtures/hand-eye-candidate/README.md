# COMM4-22 golden fixtures

Both JSON files are committed synthetic ground truth, not physical capture.
`generate.py` uses only Python stdlib matrix multiplication, rigid inverse and
axis-angle construction; it never imports OpenCV or product code. Regeneration
is optional and is not part of CTest. The C++ contract test reads committed JSON.

Each mode has the same nonidentity X and fixed transform, twelve training and
five holdout poses. Eye-in-hand constructs C=inv(X) inv(G) Y; eye-to-hand uses
C=inv(X) G Z. Domain quaternion reconstruction is an additional independent
oracle. File SHA256 identities are saved under build/comm4/COMM4-22.

These data cannot establish controller compatibility, actual robot stationarity,
optics, independent physical accuracy or activation permission.
