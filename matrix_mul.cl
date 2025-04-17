__kernel void matrixMul(
    __global const float* input_tile,       // Tile of the Input vector
    __global const float* weights_tile,    // Tile of the Weights matrix
    const int input_tile_size,             // Size of the input tile
    const int output_neurons_tile_size,    // Size of the output tile (number of neurons in this tile)
    __global float* output_tile            // Output vector tile
)
{
    // Calculate the ID of the neuron this thread will compute
    int neuron_id = get_global_id(0);
    
    // Ensure we don't process more neurons than we have in this tile
    if (neuron_id < output_neurons_tile_size) {
        float sum = 0.0f;
        
        // Perform the dot product between the input vector and the neuron's weights
        for (int i = 0; i < input_tile_size; i++) {
            sum += input_tile[i] * weights_tile[neuron_id * input_tile_size + i];
        }
        
        // Store the result in the output tile
        output_tile[neuron_id] = sum;
    }
}