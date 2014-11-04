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
        point_labels = frame.point_labels[:]

        # Overall normal image
        normals = np.zeros(depth.shape + (3,))

        # Extract each user from the depth image
        for user in frame.users:
            user_mask = label == user._v_attrs.idx
            user_depths = np.where(user_mask, depth, 0)
            user_normals = compute_normals(user_depths)
            normals = np.where(np.dstack((user_mask,) * 3), user_normals, normals)

        # Compute a normal image
        light_im = 0.5 + 0.5 * normals
        Image.fromarray(np.clip(255*light_im, 0, 255).astype(np.uint8)).save(
            '{0}-{1:05d}.png'.format(opts['<frame-prefix>'], frame_idx))

#        light_im = np.dstack((light_intensity, light_intensity, light_intensity))
#        Image.fromarray(np.clip(255*light_im, 0, 255).astype(np.uint8)).save(
#            '{0}-{1:05d}.png'.format(opts['<frame-prefix>'], frame_idx))

def compute_normals(depths):
    """Given a NxM array of depths, compute a NxMx3 array of computed normals."""

    # mask is True iff this is a valid depth pixel
    mask = depths != 0

    # convert depths to floating point
    depths = depths.astype(np.float32)

    # Use a dilation filter to "grow" depth image
    grow_depths = np.where(mask,
            depths,
            ndi.morphology.grey_dilation(depths, size=7)
    )

    # Blur depth image
    grow_depths = ndi.filters.gaussian_filter(grow_depths, 1)

    # Compute row-wise and col-wise gradients
    row_grads, col_grads = np.gradient(grow_depths)

    # Compute pseudo-3d tangent vectors
    ddx = grow_depths * 1e-3
    ddy = -(grow_depths * 1e-3)
    row_tangents = np.dstack((ddx, np.zeros_like(ddx), row_grads))
    col_tangents = np.dstack((np.zeros_like(ddy), ddy, col_grads))

    # Take cross product to compute normal
    normals = np.cross(col_tangents, row_tangents)

    # Normalise normals
    norm_lens = np.sqrt(np.sum(normals ** 2, axis=-1))
    norm_lens[norm_lens == 0] = 1 # Don't touch zero-length normals
    for i in range(3):
        normals[...,i] /= norm_lens
        normals[...,i] *= mask

    return normals

if __name__ == '__main__':
    main()

