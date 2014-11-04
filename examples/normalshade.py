#!/usr/bin/env python
#
# An example script for shading user depth silhouettes with some simple "fake"
# shading.
"""
Usage:
    normalshade.py (-h | --help)
    normalshade.py [--verbose] <logfile> <frame-prefix>

Options:
    -h, --help      Show a brief usage summary.
    -v, --verbose   Increase verbosity of output.
"""
import logging
import docopt
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
import scipy.ndimage as ndi
import tables

LOG = logging.getLogger()

def main():
    """Main entry point."""
    opts = docopt.docopt(__doc__)
    logging.basicConfig(
        level=logging.INFO if opts['--verbose'] else logging.WARN
    )

    LOG.info('Opening log file {0}'.format(opts['<logfile>']))
    log_root = tables.open_file(opts['<logfile>']).root

    for frame in log_root.frames:
        frame_idx = frame._v_attrs.idx
        if frame_idx % 30 == 0:
            LOG.info('Processing frame {0}...'.format(frame_idx))

        # Copy depth and label image to numpy array
        depth, label = frame.depth[:], frame.label[:]

        # Copy points to numpy array
        points = frame.points[:]

        # Create NxMx3 "point map"
        point_map = np.zeros(depth.shape + (3,))
        point_map[depth != 0, :] = points

        # Overall normal image
        normals = np.zeros(depth.shape + (3,))

        # Extract each user from the depth image
        for user in frame.users:
            user_mask = label == user._v_attrs.idx
            user_points = np.where(np.dstack((user_mask,)*3), point_map, 0)
            user_normals = compute_normals(user_points, user_mask)
            normals = np.where(np.dstack((user_mask,) * 3), user_normals, normals)

        # Compute a normal image
        light_im = 0.5 + 0.5 * normals
        Image.fromarray(np.clip(255*light_im, 0, 255).astype(np.uint8)).save(
            '{0}-{1:05d}.png'.format(opts['<frame-prefix>'], frame_idx))

def compute_normals(points, mask):
    """
    Given a NxMx3 array of points, compute a NxMx3 array of computed normals.
    mask should be a NxM array of booleans which are true iff the corresponding
    pixels in points are valid.

    """
    # Mask invalid points
    points = np.copy(points)
    for i in range(3):
        points[np.logical_not(mask), i] = points[..., i].min()

    # Use a dilation filter to "grow" each point
    grow_points = np.copy(points)
    for i in range(4):
        grow_points = np.where(np.dstack((mask,)*3),
                grow_points,
                ndi.filters.maximum_filter(grow_points, size=(3,3,1))
        )

    # Blur point image
    for i in range(3):
        grow_points[...,i] = ndi.filters.gaussian_filter(grow_points[...,i], 1)

    # Compute row-wise and col-wise gradients
    dxdu, dxdv = np.gradient(grow_points[...,0])
    dydu, dydv = np.gradient(grow_points[...,1])
    dzdu, dzdv = np.gradient(grow_points[...,2])

    # Compute 3d tangent vectors
    row_tangents = np.dstack((dxdu, dydu, dzdu))
    col_tangents = -np.dstack((dxdv, dydv, dzdv)) # NB: -ve since v points along -ve y

    # Take cross product to compute normal
    normals = np.cross(row_tangents, col_tangents)

    # Normalise normals
    norm_lens = np.sqrt(np.sum(normals ** 2, axis=-1))
    norm_lens[norm_lens == 0] = 1 # Don't touch zero-length normals
    for i in range(3):
        normals[...,i] /= norm_lens
        normals[...,i] *= mask

    return normals

if __name__ == '__main__':
    main()

