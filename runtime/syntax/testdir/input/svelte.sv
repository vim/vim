<!-- VIM_TEST_SETUP set filetype=svelte -->
<!-- Svelte Syntax Test File -->
<!-- Maintainer: AI Assistant -->
<!-- Last Change: 2026 Sep 08 -->

<!-- Basic HTML Elements -->
<div></div>
<span></span>
<p>Paragraph text</p>
<h1>Heading 1</h1>
<h2>Heading 2</h2>
<ul>
  <li>Item 1</li>
  <li>Item 2</li>
</ul>

<!-- Svelte Component Elements -->
<svelte:options runes={true} />

<svelte:head>
  <title>Page Title</title>
  <meta name="description" content="A Svelte page">
</svelte:head>

<svelte:window on:load={handleLoad} on:resize={handleResize} />

<svelte:body on:click={handleClick} />

<svelte:document on:keydown={handleKeydown} />

<svelte:element this={tagName} on:click={handleClick}>
  Dynamic element
</svelte:element>

<svelte:boundary on:error={handleError}>
  Error boundary content
</svelte:boundary>

<svelte:component this={CurrentComponent} prop={value} />

<svelte:fragment>
  Fragment content
</svelte:fragment>

<svelte:self />

<!-- Element Directives -->
<button on:click={handleClick}>Click me</button>
<button on:click|once|preventDefault={handleClick}>With modifiers</button>

<!-- Svelte 5 Event Attributes -->
<button onclick={handleClick}>Svelte 5 click</button>
<button onclick={handleClick} ondblclick={handleDblClick}>Multiple events</button>
<input oninput={(e) => name = e.target.value} />

<!-- Store Auto-subscriptions (template usage) -->
<span>Count: {$count}</span>
<span>Doubled: {$doubled}</span>

<!-- Bindable Directives -->

<input bind:value={name} />
<input bind:value={name} bind:group={selected} />
<textarea bind:value={text}></textarea>
<select bind:value={selected}>
  <option value="a">A</option>
  <option value="b">B</option>
</select>

<div class:active={isActive} class:disabled={isDisabled}>Conditional classes</div>
<div class:large={size === 'large'}>Large class</div>

<div use:tooltip={{ text: 'Hello' }}>Tooltip</div>
<div use:clickOutside={handleClickOutside}>Click outside</div>

<div transition:fade={{ duration: 300 }}>Fade transition</div>
<div in:fly={{ y: 20 }} out:fly={{ y: -20 }}>Fly transition</div>
<div in:slide out:fade>Slide in, fade out</div>

<div animate:flip={{ duration: 300 }}>Flip animation</div>

<div let:item>{item.name}</div>
<div slot:header>Header content</div>
<div style:color={textColor} style:font-size={fontSize}>Styled</div>

<!-- Data Attributes -->
<div data-testid="my-element">Data attribute</div>
<div aria-label="Close">Aria attribute</div>

<!-- Spread and Shorthand Attributes -->
<div {...spreadProps}>Spread attributes</div>
<button {disabled} {type}>Shorthand attributes</button>

<!-- String Attributes -->
<div class="container">Quoted class</div>
<div title="{name}">Mustache in attribute</div>
<input type="text" placeholder="Enter text" />

<!-- Unquoted Attributes -->
<div class=unquoted>Unquoted</div>

<!-- Svelte Control Flow: If/Else -->
{#if loggedIn}
  <p>Welcome, {username}!</p>
{:else if isGuest}
  <p>Welcome, guest!</p>
{:else}
  <p>Please log in.</p>
{/if}

{#if items.length > 0}
  <ul>
    {#each items as item, index (item.id)}
      <li>{index}: {item.name}</li>
    {/each}
  </ul>
{/if}

<!-- Svelte Control Flow: Each -->
{#each colors as color, i}
  <span style:background={color}>{i}: {color}</span>
{/each}

{#each items as item (item.id)}
  <div>{item.name}</div>
{:else}
  <p>No items found.</p>
{/each}

<!-- Svelte Control Flow: Await -->
{#await promise}
  <p>Loading...</p>
{:then data}
  <p>Result: {data}</p>
{:catch error}
  <p>Error: {error.message}</p>
{/await}

{#await fetchData()}
  <p>Fetching...</p>
{:then result}
  <p>Got: {result}</p>
{:catch err}
  <p>Failed: {err}</p>
{/await}

<!-- Svelte Control Flow: Key -->
{#key value}
  <div>This updates when value changes</div>
{/key}

<!-- Svelte Control Flow: Snippet (Svelte 5) -->
{#snippet greet(name)}
  <p>Hello, {name}!</p>
{/snippet}

{#snippet count(n)}
  <span>Count: {n}</span>
{/snippet}

{@render greet('World')}
{@render count(42)}

<!-- Svelte Special Blocks -->
{@html rawHtml}
{@html '<p>Raw HTML content</p>'}

{@const x = 10}
<p>Constant value: {x}</p>

{@debug variable}
{@debug name, age, email}

<!-- Svelte Attach (Svelte 5.29) -->
<div {@attach tooltip}>Attached</div>
<div {@attach setup}>Setup attachment</div>

<!-- Svelte Declaration Tags (Svelte 5.56) -->
{const area = 10 * 20}
{let temp = $state(0)}

<!-- JavaScript in Script Block -->
<script>
  // Variables
  let count = $state(0);
  let items = $state([]);
  let name = $state('World');
  let isVisible = $state(true);
  let selected = $state(null);
  let textColor = $state('#333');
  let fontSize = $state('16px');
  let loggedIn = $state(false);
  let isGuest = $state(false);
  let username = $state('User');
  let rawHtml = $state('<em>Raw</em>');
  let value = $state(0);
  let tagName = $state('div');

  // Props and bindable
  let props = $props();
  let el = $bindable();

  // Derived values
  let doubled = $derived(count * 2);
  let total = $derived(items.reduce((sum, item) => sum + item.price, 0));

  // Effects
  $effect(() => {
    console.log('Count changed:', count);
    return () => {
      console.log('Cleanup');
    };
  });

  // Inspect and host runes
  $inspect(count, name);
  $inspect(count).with((type, val) => console.log(type, val));
  let el2 = $host();

  // Dot-notation rune variants
  let rawState = $state.raw({ name: 'test' });
  let snapshot = $state.snapshot(rawState);
  let eagerValue = $state.eager(count);
  let total2 = $derived.by(() => {
    return items.reduce((sum, item) => sum + item.price, 0);
  });
  $effect.pre(() => {
    console.log('Before DOM update');
  });
  let tracked = $effect.tracking();
  let pending = $effect.pending();
  const cleanup = $effect.root(() => {
    $effect(() => {
      console.log('Nested effect');
    });
    return () => console.log('Cleanup');
  });
  const uid = $props.id();

  // Functions
  function handleClick() {
    count += 1;
  }

  function handleLoad() {
    console.log('Window loaded');
  }

  function handleResize() {
    console.log('Window resized');
  }

  function handleKeydown(event) {
    console.log('Key pressed:', event.key);
  }

  function handleClickOutside() {
    console.log('Clicked outside');
  }

  // Promises
  let promise = new Promise((resolve) => {
    setTimeout(() => resolve('Hello!'), 1000);
  });

  async function fetchData() {
    const response = await fetch('/api/data');
    return response.json();
  }

  // Store subscriptions
  let todos = $state([]);

  function addTodo(text) {
    todos = [...todos, { id: Date.now(), text, done: false }];
  }

  function removeTodo(id) {
    todos = todos.filter(t => t.id !== id);
  }

  // Colors array for testing each block
  let colors = $state(['red', 'green', 'blue', 'yellow']);

  // Size variable for conditional class
  let size = $state('medium');
</script>

<!-- TypeScript in Script Block -->
<script lang="ts">
  interface Todo {
    id: number;
    text: string;
    done: boolean;
  }

  type Status = 'loading' | 'success' | 'error';

  let status: Status = $state('loading');
  let todos: Todo[] = $state([]);

  function handleTodoClick(todo: Todo): void {
    todo.done = !todo.done;
  }

  const API_BASE: string = 'https://api.example.com';
</script>

<!-- Module Script (SvelteKit) -->
<script module>
  export function load({ url }) {
    return { props: { url } };
  }
</script>

<!-- CSS in Style Block -->
<style>
  :global(body) {
    font-family: Arial, sans-serif;
    margin: 0;
    padding: 0;
  }

  .container {
    max-width: 1200px;
    margin: 0 auto;
    padding: 1rem;
  }

  .active {
    background-color: #e0e0e0;
    font-weight: bold;
  }

  .disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  .large {
    font-size: 1.5rem;
  }

  button {
    padding: 0.5rem 1rem;
    border: 1px solid #ccc;
    border-radius: 4px;
    cursor: pointer;
  }

  button:hover {
    background-color: #f0f0f0;
  }

  ul {
    list-style: none;
    padding: 0;
  }

  li {
    padding: 0.25rem 0;
  }

  p {
    margin: 0.5rem 0;
  }

  :global(.dark) {
    color: white;
    background-color: #333;
  }
</style>

<!-- Nested Elements with Directives -->
<div class="wrapper" use:tooltip={{ text: 'Wrapper' }}>
  <button
    on:click={handleClick}
    class:active={count > 0}
    disabled={isDisabled}
  >
    Count: {count}
  </button>

  {#if isVisible}
    <div transition:fade={{ duration: 200 }}>
      <p>Visible content</p>
    </div>
  {/if}
</div>

<!-- Comments Inside Template -->
<!-- This is an HTML comment -->
{#each items as item}
  <!-- Item content -->
  <div>{item.name}</div>
{/each}

<!-- Mixed Content -->
<footer>
  <p>&copy; {new Date().getFullYear()} My App</p>
  <nav>
    <a href="/">Home</a>
    <a href="/about">About</a>
  </nav>
</footer>
